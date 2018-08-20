#include <catch2/catch.hpp>
#include "CSV.hpp"

// TODO: Test with random data.

const char test01_path[] = "./CSV-tests/test-01.csv";
const char test02_path[] = "./CSV-tests/test-02.csv";

using namespace std;

// TEST_CASE("Output csv file to stream", "[csv]") {
//     CSV csv(test02_path);
    // csv.write(cout);
// }

TEST_CASE("Load csv file from disk", "[csv]") {
    CSV csv(test02_path);
    REQUIRE( csv.nCols() == 3 );
    REQUIRE( csv.nRows() == 4 );
}

TEST_CASE("Retrieve columns by index", "[csv]") {
    CSV csv(test02_path);
    vector<string> expokt = {"1,2",
                             "3\"4",
                             "5,6"};
    for (int i = 0; i < csv.nCols(); i++) {
        auto vec = csv.getColCells(i);
        const string& expect_column = expokt[i];
        for (auto column : *vec) {
            REQUIRE( *column == expect_column );
        }
        delete vec;
    }
}

TEST_CASE("Retrieve columns by name", "[csv]") {
    CSV csv(test02_path);
    vector<string> names = {"alpha,bravo",
                            "charlie,delta",
                            "echo,foxtrot",};
    vector<string> expokt = {"1,2",
                             "3\"4",
                             "5,6"};
    for (int i = 0; i < csv.nCols(); i++) {
        auto vec = csv.getColCells(names[i]);
        const string& expect_column = expokt[i];
        for (auto column : *vec) {
            REQUIRE( *column == expect_column );
        }
        delete vec;
    }
}
