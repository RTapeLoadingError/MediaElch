#include "test/test_helpers.h"

#include "scrapers/movie/hotmovies/HotMovies.h"
#include "scrapers/movie/hotmovies/HotMoviesSearchJob.h"
#include "test/scrapers/testScraperHelpers.h"

#include <chrono>

using namespace std::chrono_literals;
using namespace mediaelch::scraper;

static HotMoviesApi& getHotMoviesApi()
{
    static auto api = std::make_unique<HotMoviesApi>();
    return *api;
}

/// @brief Loads movie data synchronously
static void loadHotMoviesSync(HotMovies& scraper, QHash<MovieScraper*, MovieIdentifier> ids, Movie& movie)
{
    const auto infos = scraper.meta().supportedDetails;
    loadDataSync(scraper, ids, movie, infos);
}

TEST_CASE("HotMovies returns valid search results", "[HotMovies][search]")
{
    SECTION("Search by movie name returns correct results")
    {
        MovieSearchJob::Config config{"Magic Mike XXXL", mediaelch::Locale::English};
        auto* searchJob = new HotMoviesSearchJob(getHotMoviesApi(), config);
        const auto scraperResults = searchMovieScraperSync(searchJob).first;

        REQUIRE(scraperResults.length() >= 1);
        CHECK(scraperResults[0].title == "Magic Mike XXXL: A Hardcore Parody");
    }
}


TEST_CASE("HotMovies scrapes correct movie details", "[HotMovies][load_data]")
{
    HotMovies hm;

    SECTION("Movie has correct details")
    {
        Movie m(QStringList{}); // Movie without files
        loadHotMoviesSync(hm,
            {{nullptr, MovieIdentifier("https://www.hotmovies.com/video/292788/Magic-Mike-XXXL-A-Hardcore-Parody/")}},
            m);

        REQUIRE_THAT(m.name(), StartsWith("Magic Mike XXXL"));
        CHECK(m.imdbId() == ImdbId::NoId);
        CHECK(m.tmdbId() == TmdbId::NoId);
        CHECK(m.released().toString("yyyy") == "2015");
        REQUIRE(!m.ratings().isEmpty());
        // Rating currently no available
        // CHECK(m.ratings().first().rating == Approx(4).margin(0.5));
        CHECK(m.ratings().first().voteCount > 50);
        CHECK(m.images().posters().size() == 1);
        CHECK(m.images().backdrops().size() == 1);
        CHECK(m.runtime() == 201min);

        CHECK_THAT(m.overview(), Contains("with a great storyline"));
        CHECK(m.director() == "Brad Armstrong");

        const auto genres = m.genres();
        REQUIRE(genres.size() >= 2);
        CHECK(genres[0] == "Contemporary");
        CHECK(genres[1] == "Spoofs / Parodies");

        const auto studios = m.studios();
        REQUIRE(!studios.empty());
        CHECK(studios[0] == "Wicked Pictures");

        const auto actors = m.actors().actors();
        REQUIRE(actors.size() > 15);
        CHECK(actors[0]->name == "Adriana Chechik");
        // URL could be: "https://img2.vod.com/image2/star/163/Adriana_Chechik-163576.4.jpg
        CHECK_THAT(actors[0]->thumb, StartsWith("https://"));
        CHECK_THAT(actors[0]->thumb, Contains("163"));
        CHECK_THAT(actors[0]->thumb, Contains("Adriana_Chechik"));
        CHECK_THAT(actors[0]->thumb, EndsWith(".jpg"));
        CHECK(actors[1]->name == "Amirah Adara");
        CHECK_THAT(actors[1]->thumb, StartsWith("https://"));
        CHECK_THAT(actors[1]->thumb, Contains("Amirah_Adara"));
        CHECK_THAT(actors[1]->thumb, EndsWith(".jpg"));
    }

    SECTION("Movie has correct set")
    {
        Movie m(QStringList{}); // Movie without files
        loadHotMoviesSync(
            hm, {{nullptr, MovieIdentifier("https://www.hotmovies.com/video/214343/-M-Is-For-Mischief-Number-3/")}}, m);
        CHECK(m.name() == "\"M\" Is For Mischief Number 3");
        CHECK(m.set().name == "\"M\" Is For Mischief");
    }
}
