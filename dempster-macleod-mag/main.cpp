#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <inttypes.h>
#include <stdint.h>

/*
Implementation of MAG algorithm described in https://digital-library.theiet.org/content/journals/10.1049/ip-cds_19941191
Goal - find first integer that can not be representated with 4 adders
I believe, 32 bits would be enough, to accept the goal.
*/
using namespace std;


/*
    To not work with overflow
*/
const uint64_t MAX = 65536;

/*
    Global map of integers and their cost
*/
unordered_map<uint64_t, uint64_t> already_found;


/*
    Global map of fundamentals for each variable of cost2 and higher
*/
unordered_map<uint64_t, unordered_set<uint64_t> > fundamentals;
void add_fundamental_to(uint64_t fundamental, uint64_t to) {
    if (fundamentals.find(to) != fundamentals.end()) {
        fundamentals[to].insert(fundamental);
    } else {
        unordered_set<uint64_t> fundamentals_vector = {fundamental};
        fundamentals[to] = fundamentals_vector;
    }
}

const unordered_set<uint64_t> get_fundamentals(uint64_t of) {
    if (fundamentals.find(of) != fundamentals.end()) {
        return fundamentals[of];
    } else {
        return unordered_set<uint64_t>{};
    }
}


void update_2i(const vector<uint64_t> cost0, const vector<uint64_t> updater, vector<uint64_t>& to_update, int cost) {
    for (uint64_t i = 0; i < updater.size(); i++) {
        uint64_t upd = updater[i];
        const unordered_set<uint64_t> fundamentals_set = get_fundamentals(updater[i]);
        while(upd % 2 == 0) {
            upd /= 2;
            if (already_found.find(upd) == already_found.end()) {
                to_update.push_back(upd);
                already_found.insert({upd, cost});
                for (auto fundamental : fundamentals_set) {
                    add_fundamental_to(fundamental, upd);
                }
            }
        }
        for (uint64_t j = 0; j < cost0.size(); j++) {
            uint64_t current_value = cost0[j] * updater[i];
            if (current_value >= MAX) {
                break;
            }
            if (already_found.find(current_value) == already_found.end()) {
                to_update.push_back(current_value);
                already_found.insert({current_value, cost});
                for (auto fundamental : fundamentals_set) {
                    add_fundamental_to(fundamental, current_value);
                }
            }
        }
    }
}

/*
    Filling 0 cost vector -> 2^i
*/
vector<uint64_t> get0cost(size_t bits) {
    vector<uint64_t> cost0(bits + 1);
    for (uint64_t i = 0; i <= bits; i++) {
        cost0[i] = (1 << i);
        already_found.insert({cost0[i], 0});
    }

    return std::move(cost0);
}


/*
    Returns 0 if cost is lower or integer is incorrect
    Returns 1 if cost is the same
    Returns 2 if it is the first representation 
*/
uint64_t status_of_new_int(uint64_t new_int, uint64_t current_cost) {
    if (new_int <= 0 || new_int > MAX) {
        return 0;
    }
    if (already_found.find(new_int) == already_found.end()) {
        return 2;
    }
    if (already_found.at(new_int) < current_cost) {
        return 0;
    } else {
        return 1;
    }
}


/*
    Filling 1 cost vector -> cost0 +- 1
*/
vector<uint64_t> get1cost(const vector<uint64_t>& cost0) {
    size_t cost0size = cost0.size();
    vector<uint64_t> cost1;
    cost1.reserve(cost0size + cost0size);
    for (uint64_t i = 0; i < cost0size; i++) {
        if (status_of_new_int(cost0[i] - 1, 1)) {
            cost1.push_back(cost0[i] - 1);
            already_found.insert({cost0[i] - 1, 1});
        }

        if (status_of_new_int(cost0[i] + 1, 1)) {
            cost1.push_back(cost0[i] + 1);
            already_found.insert({cost0[i] + 1, 1});
        }
    }
    cost1.shrink_to_fit();

    update_2i(cost0, cost1, cost1, 1);
    return std::move(cost1);
}


void concider_integer_cost(uint64_t consider, vector<uint64_t> maybe_fundamental, vector<uint64_t>& int_vector, uint64_t cost) {
    if (uint64_t status = status_of_new_int(consider, cost)) {
        if (status == 2) {
            int_vector.push_back(consider);
            already_found.insert({consider, cost});
        }
        for (auto fundamental : maybe_fundamental)
            add_fundamental_to(fundamental, consider);
    }
}


/*
    Filling 2 cost vector -> {
        cost0 * cost1 +- 1
        +- cost0 +- cost1
        cost1 * cost1 +- cost1
    }
*/
vector<uint64_t> get2cost(const vector<uint64_t>& cost0, const vector<uint64_t>& cost1) {
    size_t cost0size = cost0.size();
    size_t cost1size = cost1.size();
    vector<uint64_t> cost2;
    cost2.reserve(cost0size * cost1size * 8);
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost1size; j++) {
            concider_integer_cost(cost0[i] * cost1[j] - 1, {cost1[j]}, cost2, 2);
            concider_integer_cost(cost0[i] * cost1[j] + 1, {cost1[j]}, cost2, 2);
            concider_integer_cost(cost0[i] + cost1[j], {cost1[j]}, cost2, 2);
            concider_integer_cost(cost0[i] - cost1[j], {cost1[j]}, cost2, 2);
            concider_integer_cost(-cost0[i] + cost1[j], {cost1[j]}, cost2, 2);
            //concider_integer_cost(-cost0[i] - cost1[j], {cost1[j]}, cost2, 2); // should never be added as it is < 0
            concider_integer_cost(cost0[i] * cost1[j] - cost1[j], {cost1[j]}, cost2, 2);
            concider_integer_cost(cost0[i] * cost1[j] + cost1[j], {cost1[j]}, cost2, 2);
        }
    }
    cost2.shrink_to_fit();

    update_2i(cost0, cost2, cost2, 2);
    return std::move(cost2);
}


/*
    Filling 3 cost vector -> {
        combination of cost2 + one of (cost0, it's fundamential, itself)
        combination of two cost1
    }
*/
vector<uint64_t> get3cost(const vector<uint64_t>& cost0, const vector<uint64_t>& cost1, const vector<uint64_t>& cost2) {
    size_t cost0size = cost0.size();
    size_t cost1size = cost1.size();
    size_t cost2size = cost2.size();
    vector<uint64_t> cost3;
    cost3.reserve(cost1size * cost1size * cost0size * 4 + cost0size * cost2size * 12);

    cout << "Counting combination of two cost1" << endl;
    // combination of two cost1
    for (uint64_t i = 0; i < cost1size; i++) {
        for (uint64_t j = 0; j < cost1size; j++) {
            if (i == j) {
                continue;
            }
            for (uint64_t k = 0; k < cost0size; k++) {
                concider_integer_cost(cost0[k] * cost1[i] - cost1[j], {cost1[i], cost1[j]}, cost3, 3);
                concider_integer_cost(cost0[k] * cost1[i] + cost1[j], {cost1[i], cost1[j]}, cost3, 3);
                concider_integer_cost(-cost0[k] * cost1[i] + cost1[j], {cost1[i], cost1[j]}, cost3, 3);
              //  concider_integer_cost(-cost0[k] * cost1[i] - cost1[j], {cost1[i], cost1[j]}, cost3, 3); // should never be added as it is < 0
            }
        }
    }

    cout << "Counting combination fo cost2 + cost0 and cost2 + cost2" << endl;
    // combination fo cost2 + cost0 and cost2 + cost2
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost2size; j++) {
            // cost2 + cost0
            concider_integer_cost(cost0[i] * cost2[j] - 1, {cost2[j]}, cost3, 3);
            concider_integer_cost(cost0[i] * cost2[j] + 1, {cost2[j]}, cost3, 3);
            concider_integer_cost(cost0[i] + cost2[j], {cost2[j]}, cost3, 3);
            concider_integer_cost(cost0[i] - cost2[j], {cost2[j]}, cost3, 3);
            concider_integer_cost(-cost0[i] + cost2[j], {cost2[j]}, cost3, 3);
           // concider_integer_cost(-cost0[i] - cost2[j], {cost2[j]}, cost3, 3); // should never be added as it is < 0
            // cost2 + cost2
            concider_integer_cost(cost0[i] * cost2[j] - cost2[j], {cost2[j]}, cost3, 3);
            concider_integer_cost(cost0[i] * cost2[j] + cost2[j], {cost2[j]}, cost3, 3);
        }
    }

    cout << "Counting combination fo cost2 + fundamental" << endl;
    // combination fo cost2 + fundamental
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost2size; j++) {
            const unordered_set<uint64_t> fundamentals_set = get_fundamentals(cost2[i]);
            for (auto fundamental : fundamentals_set) {
                concider_integer_cost(cost0[i] * cost2[j] - fundamental, {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * cost2[j] + fundamental, {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * fundamental - cost2[j], {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * fundamental + cost2[j], {cost2[j], fundamental}, cost3, 3);
            }

        }
    }

    cost3.shrink_to_fit();

    update_2i(cost0, cost3, cost3, 3);
    return std::move(cost3);
}



void concider_terminating(uint64_t consider, vector<uint64_t>& consider_terminating) {
    if (status_of_new_int(consider, 2)) {
        consider_terminating.push_back(consider);
    }
}

void add_terminating(uint64_t cost0, uint64_t cost1, vector<pair<uint64_t, uint64_t> >& terminating) {
    vector<uint64_t> maybe_terminating;
    maybe_terminating.reserve(8);
    concider_terminating(cost0 * cost1 + 1, maybe_terminating);
    concider_terminating(cost0 * cost1 - 1, maybe_terminating);
    concider_terminating(cost0 + cost1, maybe_terminating);
    concider_terminating(cost0 - cost1, maybe_terminating);
    concider_terminating(-cost0 + cost1, maybe_terminating);
    // concider_terminating(-cost0 - cost1, maybe_terminating);
    concider_terminating(cost0 * cost1 + cost1, maybe_terminating);
    concider_terminating(cost0 * cost1 - cost1, maybe_terminating);
    for (uint64_t i = 0; i < maybe_terminating.size(); i++) {
        for (uint64_t j = i; j < maybe_terminating.size(); j++) {
            terminating.emplace_back(maybe_terminating[i], maybe_terminating[j]);
        }
    }
}

/*
    Filling 4 cost vector -> {
        combination of cost3 + one of (cost0, it's fundamential, itself)
        combination of cost2 + cost1
        sum cost3 with 2 terminating vertices
    }
*/
vector<uint64_t> get4cost(const vector<uint64_t>& cost0, const vector<uint64_t>& cost1, const vector<uint64_t>& cost2, const vector<uint64_t>& cost3) {
    size_t cost0size = cost0.size();
    size_t cost1size = cost1.size();
    size_t cost2size = cost2.size();
    size_t cost3size = cost3.size();
    vector<uint64_t> cost4;
    cost4.reserve(cost1size * cost1size * cost0size * 8);

    cout << "Counting combination of cost2 + cost1" << endl;
    // combination of cost2 + cost1
    for (uint64_t i = 0; i < cost1size; i++) {
        for (uint64_t j = 0; j < cost2size; j++) {
            for (uint64_t k = 0; k < cost0size; k++) {
                concider_integer_cost(cost0[k] * cost2[j] - cost1[i], {}, cost4, 4);
                concider_integer_cost(cost0[k] * cost2[j] + cost1[i], {}, cost4, 4);
                concider_integer_cost(-cost0[k] * cost2[j] + cost1[i], {}, cost4, 4);
            //    concider_integer_cost(-cost0[k] * cost2[j] - cost1[i], {}, cost4, 4); // should never be added as it is < 0
                concider_integer_cost(cost0[k] * cost1[i] - cost2[j], {}, cost4, 4);
                concider_integer_cost(cost0[k] * cost1[i] + cost2[j], {}, cost4, 4);
                concider_integer_cost(-cost0[k] * cost1[i] + cost2[j], {}, cost4, 4);
              //  concider_integer_cost(-cost0[k] * cost1[i] - cost2[j], {}, cost4, 4); // should never be added as it is < 0
            }
        }
    }


    cout << "Counting combination fo cost3 + cost0 and cost3 + cost3" << endl;
    // combination fo cost3 + cost0 and cost3 + cost3
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost3size; j++) {
            // cost3 + cost0
            concider_integer_cost(cost0[i] * cost3[j] - 1, {}, cost4, 4);
            concider_integer_cost(cost0[i] * cost3[j] + 1, {}, cost4, 4);
            concider_integer_cost(cost0[i] + cost3[j], {}, cost4, 4);
            concider_integer_cost(cost0[i] - cost3[j], {}, cost4, 4);
            concider_integer_cost(-cost0[i] + cost3[j], {}, cost4, 4);
          //  concider_integer_cost(-cost0[i] - cost3[j], {}, cost4, 4); // should never be added as it is < 0
            // cost3 + cost2
            concider_integer_cost(cost0[i] * cost3[j] - cost3[j], {}, cost4, 4);
            concider_integer_cost(cost0[i] * cost3[j] + cost3[j], {}, cost4, 4);
        }
    }


    cout << "Counting combination fo cost3 + fundamental" << endl;
    // combination fo cost3 + fundamental
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost3size; j++) {
            const unordered_set<uint64_t> fundamentals_set = get_fundamentals(cost3[i]);
            for (auto fundamental : fundamentals_set) {
                concider_integer_cost(cost0[i] * cost3[j] - fundamental, {}, cost4, 4);
                concider_integer_cost(cost0[i] * cost3[j] + fundamental, {}, cost4, 4);
                concider_integer_cost(cost0[i] * fundamental - cost3[j], {}, cost4, 4);
                concider_integer_cost(cost0[i] * fundamental + cost3[j], {}, cost4, 4);
            }

        }
    }

    cout << "Finding cost3 with 2 terminating vertices" << endl;
    // finding cost3 with 2 terminating vertices
    vector<pair<uint64_t, uint64_t>> terminating;
    terminating.reserve(cost0size * cost1size * 16);
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost1size; j++) {
            add_terminating(cost0[i], cost1[j], terminating);
        }
    }

    cout << "Counting combinations of terminating vertices" << endl;
    // combinations of terminating vertices
    for (uint64_t i = 0; i < terminating.size(); i++) {
        uint64_t first = terminating[i].first;
        uint64_t second = terminating[i].second;
        for (int j = 0; j < cost0size; j++) {
            concider_integer_cost(cost0[j] * first - second, {}, cost4, 4);
            concider_integer_cost(cost0[j] * first + second, {}, cost4, 4);
          //  concider_integer_cost(- cost0[j] * first - second, {}, cost4, 4);
            concider_integer_cost(- cost0[j] * first + second, {}, cost4, 4);
            concider_integer_cost(cost0[j] * second - first, {}, cost4, 4);
            concider_integer_cost(cost0[j] * second + first, {}, cost4, 4);
        //    concider_integer_cost(- cost0[j] * second - first, {}, cost4, 4);
            concider_integer_cost(- cost0[j] * second + first, {}, cost4, 4);
        }
    }

    update_2i(cost0, cost4, cost4, 4);
    return std::move(cost4);
}


int main() {

    vector<uint64_t> cost0 = get0cost(16);
    cout << "cost0 filled" << endl;

    vector<uint64_t> cost1 = get1cost(cost0);
    cout << "cost1 filled" << endl;

    vector<uint64_t> cost2 = get2cost(cost0, cost1);
    cout << "cost2 filled" << endl;

    vector<uint64_t> cost3 = get3cost(cost0, cost1, cost3);
    cout << "cost3 filled" << endl;

    vector<uint64_t> cost4 = get4cost(cost0, cost1, cost3, cost4);
    cout << "cost4 filled" << endl;

    cout << already_found.size() << endl;
    vector<pair<uint64_t,uint64_t> > ans;
    ans.reserve(already_found.size());
    for (auto x : already_found) {
        ans.push_back(x);
    }
    sort(ans.begin(), ans.end());
    int now = 1;
    for (int i = 0; i < ans.size(); i++) {
        if (ans[i].first != i + now) {
            cout << "First number is: " << i + 1 << endl;
            break;
        }
    }
    for (uint64_t i = 0; i < ans.size(); i++) {
        cout << ans[i].first << ' ' << ans[i].second << endl;
    }

}