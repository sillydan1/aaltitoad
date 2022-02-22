#include "ntta.h"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

void ntta_t::tick() {

}

ntta_t::diff_t ntta_t::tick() const {
    return {};
}

void ntta_t::tock() {

}

symbol_map_t ntta_t::tock() const {
    return {};
}

void ntta_t::operator+=(const ntta_t::diff_t &diff) {

}

void ntta_t::operator+=(const symbol_map_t &diff) {

}

#pragma clang diagnostic pop
