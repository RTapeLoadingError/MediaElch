#include "MusicFileSearcher.h"

#include "globals/Manager.h"
#include "globals/MessageIds.h"
#include "log/Log.h"
#include "music/Album.h"
#include "music/Artist.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent>

MusicFileSearcher::MusicFileSearcher(QObject* parent) :
    QObject(parent), m_progressMessageId{Constants::MusicFileSearcherProgressMessageId}, m_aborted{false}
{
}

void MusicFileSearcher::setMusicDirectories(QVector<SettingsDir> directories)
{
    m_directories.clear();
    for (const SettingsDir& dir : directories) {
        if (Settings::instance()->advanced()->isFolderExcluded(dir.path.dirName())) {
            qCWarning(generic) << "[MusicFileSearcher] Music directory is excluded by advanced settings! "
                                  "Is this intended? Directory:"
                               << dir.path.path();
            continue;
        }

        if (!dir.path.isReadable()) {
            qCDebug(generic) << "[MusicFileSearcher] Music directory is not redable, skipping:" << dir.path.path();
            continue;
        }

        qCDebug(generic) << "[MusicFileSearcher] Adding music directory" << dir.path.path();
        m_directories.append(dir);
    }
}

void MusicFileSearcher::reload(bool force)
{
    m_aborted = false;

    emit searchStarted(tr("Searching for Music..."));
    Manager::instance()->musicModel()->clear();

    QVector<Artist*> artists;
    QVector<Artist*> artistsFromDb;
    QVector<Album*> albums;
    QVector<Album*> albumsFromDb;

    if (force) {
        Manager::instance()->database()->clearAllArtists();
    }

    QMap<Artist*, mediaelch::DirectoryPath> artistPaths;
    QMap<Album*, mediaelch::DirectoryPath> albumPaths;
    for (const SettingsDir& dir : asConst(m_directories)) {
        if (m_aborted) {
            break;
        }
        if (dir.disabled) {
            continue;
        }

        if (dir.autoReload) {
            Manager::instance()->database()->clearArtistsInDirectory(mediaelch::DirectoryPath(dir.path));
        }

        if (dir.autoReload || force) {
            QDirIterator it(dir.path.path(), QDir::NoDotAndDotDot | QDir::Dirs, QDirIterator::FollowSymlinks);
            while (it.hasNext()) {
                if (m_aborted) {
                    break;
                }

                it.next();

                if (Settings::instance()->advanced()->isFolderExcluded(it.fileInfo().dir().dirName())) {
                    continue;
                }

                emit currentDir(it.fileInfo().baseName());
                auto* artist = new Artist(mediaelch::DirectoryPath(it.filePath()), this);
                artist->setName(it.fileInfo().baseName());
                artists.append(artist);
                artistPaths.insert(artist, mediaelch::DirectoryPath(dir.path));

                QDirIterator itAlbums(it.filePath(), QDir::NoDotAndDotDot | QDir::Dirs, QDirIterator::FollowSymlinks);
                while (itAlbums.hasNext()) {
                    itAlbums.next();

                    if (Settings::instance()->advanced()->isFolderExcluded(itAlbums.fileInfo().dir().dirName())) {
                        continue;
                    }

                    if (itAlbums.fileInfo().baseName() == "extrafanart") {
                        continue;
                    }
                    if (itAlbums.fileInfo().baseName() == "extrathumbs") {
                        continue;
                    }

                    auto* album = new Album(mediaelch::DirectoryPath(itAlbums.filePath()), this);
                    album->setTitle(itAlbums.fileInfo().baseName());
                    album->setArtistObj(artist);
                    artist->addAlbum(album);
                    albums.append(album);
                    albumPaths.insert(album, mediaelch::DirectoryPath(dir.path));
                }
            }
        } else {
            QVector<Artist*> artistsInPath =
                Manager::instance()->database()->artistsInDirectory(mediaelch::DirectoryPath(dir.path));
            for (Artist* artist : artistsInPath) {
                if (artistsFromDb.count() % 20 == 0) {
                    emit currentDir(artist->path().toString().mid(dir.path.path().length()));
                }
                QVector<Album*> albumsOfArtist = Manager::instance()->database()->albums(artist);
                artistsFromDb.append(artist);
                albumsFromDb.append(albumsOfArtist);
            }
        }
    }

    emit currentDir("");
    emit searchStarted(tr("Loading Music..."));

    int current = 0;
    int max = qsizetype_to_int(artists.length() + albums.length() + artistsFromDb.length() + albumsFromDb.length());

    Manager::instance()->database()->transaction();
    for (Artist* artist : artists) {
        if (m_aborted) {
            Manager::instance()->database()->commit();
            return;
        }
        artist->controller()->loadData(Manager::instance()->mediaCenterInterface(), true);
        if (current % 20 == 0) {
            emit currentDir(artist->name());
        }
        emit progress(++current, max, m_progressMessageId);
        Manager::instance()->database()->add(artist, artistPaths.value(artist));
    }
    for (Album* album : albums) {
        if (m_aborted) {
            Manager::instance()->database()->commit();
            return;
        }
        album->controller()->loadData(Manager::instance()->mediaCenterInterface(), true);
        if (current % 20 == 0) {
            emit currentDir(album->artist() + "/" + album->title());
        }
        emit progress(++current, max, m_progressMessageId);
        Manager::instance()->database()->add(album, albumPaths.value(album));
    }
    Manager::instance()->database()->commit();

    QtConcurrent::blockingMapped(artistsFromDb, MusicFileSearcher::loadArtistData);
    QtConcurrent::blockingMapped(albumsFromDb, MusicFileSearcher::loadAlbumData);

    artists.append(artistsFromDb);
    albums.append(albumsFromDb);

    QMap<Artist*, MusicModelItem*> artistModelItems;
    for (Artist* artist : artists) {
        MusicModelItem* artistItem = Manager::instance()->musicModel()->appendChild(artist);
        artistModelItems.insert(artist, artistItem);
    }
    for (Album* album : albums) {
        MusicModelItem* artistItem = artistModelItems.value(album->artistObj(), nullptr);
        if (artistItem == nullptr) {
            qCWarning(generic) << "Artist item was not found for album" << album->path();
            continue;
        }
        artistItem->appendChild(album);
    }

    if (!m_aborted) {
        emit musicLoaded();
    }
}

void MusicFileSearcher::abort()
{
    m_aborted = true;
}

Artist* MusicFileSearcher::loadArtistData(Artist* artist)
{
    artist->controller()->loadData(Manager::instance()->mediaCenterInterface(), false, false);
    return artist;
}

Album* MusicFileSearcher::loadAlbumData(Album* album)
{
    album->controller()->loadData(Manager::instance()->mediaCenterInterface(), false, false);
    return album;
}
