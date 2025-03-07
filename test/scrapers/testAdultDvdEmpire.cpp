#include "test/test_helpers.h"

#include "scrapers/movie/adultdvdempire/AdultDvdEmpire.h"

#include <chrono>

using namespace std::chrono_literals;
using namespace mediaelch::scraper;

/// @brief Loads movie data synchronously
void loadAdultDvdEmpireSync(AdultDvdEmpire& scraper, QHash<MovieScraper*, MovieIdentifier> ids, Movie& movie)
{
    const auto infos = scraper.meta().supportedDetails;
    loadDataSync(scraper, ids, movie, infos);
}


TEST_CASE("AdultDvdEmpire scrapes correct movie details", "[AdultDvdEmpire][load_data]")
{
    AdultDvdEmpire hm;

    SECTION("Movie has correct details for DVD movie")
    {
        Movie m(QStringList{}); // Movie without files
        loadAdultDvdEmpireSync(hm, {{nullptr, MovieIdentifier("/1745335/magic-mike-xxxl-porn-movies.html")}}, m);

        CHECK_THAT(m.name(), StartsWith("Magic Mike XXXL"));
        CHECK(m.imdbId() == ImdbId::NoId);
        CHECK(m.tmdbId() == TmdbId::NoId);
        CHECK(m.released().toString("yyyy") == "2015");

        CHECK(m.images().posters().size() == 1);
        CHECK(m.images().backdrops().size() > 70);
        CHECK(m.runtime() == 201min);

        CHECK_THAT(m.overview(), Contains("Award-Winning Director Brad Armstrong brings you"));
        CHECK(m.director() == "Brad Armstrong");

        const auto genres = m.genres();
        REQUIRE(genres.size() >= 10);
        CHECK(genres[0] == "Big Budget");
        CHECK_THAT(genres[1], StartsWith("Big"));

        const auto studios = m.studios();
        REQUIRE(!studios.empty());
        CHECK(studios[0] == "Wicked Pictures");

        const auto actors = m.actors().actors();
        REQUIRE(actors.size() > 15);
        bool foundActor = false;
        QString actorThumb;
        for (const auto* actor : actors) {
            if (actor->name == "Adriana Chechik") {
                foundActor = true;
                actorThumb = actor->thumb;
                break;
            }
        }
        CHECK(foundActor);
        CHECK(actorThumb == "https://imgs1cdn.adultempire.com/actors/652646h.jpg");
    }

    SECTION("Movie has correct details for VOD movie")
    {
        Movie m(QStringList{}); // Movie without files
        loadAdultDvdEmpireSync(hm, {{nullptr, MovieIdentifier("/1670507/50-shades-of-pink-porn-videos.html")}}, m);

        CHECK_THAT(m.name(), StartsWith("50 Shades Of Pink"));
        CHECK(m.released().toString("yyyy") == "2013");

        CHECK(m.images().posters().size() == 1);
        CHECK(m.images().backdrops().size() >= 40);
        CHECK(m.runtime() == 113min);

        CHECK_THAT(m.overview(), StartsWith("Anna Pink is a successful law attorney"));

        const auto genres = m.genres();
        REQUIRE(genres.size() >= 9);
        CHECK(genres[0] == "BDSM");

        const auto studios = m.studios();
        REQUIRE(!studios.empty());
        CHECK_THAT(m.studios().first(), StartsWith("Pure"));

        const auto actors = m.actors().actors();
        REQUIRE(actors.size() > 5);
        bool foundActor = false;
        QString actorThumb;
        for (const auto* actor : actors) {
            if (actor->name == "Alyssa Divine") {
                foundActor = true;
                actorThumb = actor->thumb;
                break;
            }
        }
        CHECK(foundActor);
        CHECK(actorThumb == "https://imgs1cdn.adultempire.com/actors/652515h.jpg");
    }
}
