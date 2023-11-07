#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <inttypes.h>
#include <stdint.h>
#include <set>

/*
Implementation of MAG algorithm described in https://digital-library.theiet.org/content/journals/10.1049/ip-cds_19941191
Goal - find first integer that can not be representated with 4 adders
I believe, 16 bits would be enough, to accept the goal.
*/
using namespace std;


/*
    To not work with overflow
*/
const uint64_t MAX = 1048576;

/*
    Global map of integers and their cost
*/
unordered_map<uint64_t, uint64_t> already_found;


/*
    Global map of fundamentals for each variable of cost2 and higher
*/
unordered_map<uint64_t, unordered_set<uint64_t> > fundamentals[5];
void add_fundamental_to(uint64_t fundamental, uint64_t to, int lvl) {
    while (fundamental % 2 == 0) {
        fundamental /= 2;
    }
    while (to % 2 == 0) {
        to /= 2;
    }
    if (fundamentals[lvl].find(to) != fundamentals[lvl].end()) {
        fundamentals[lvl][to].insert(fundamental);
    } else {
        unordered_set<uint64_t> fundamentals_vector = {fundamental};
        fundamentals[lvl][to] = fundamentals_vector;
    }
}

const unordered_set<uint64_t> get_fundamentals(uint64_t of, int lvl) {
    while (of % 2 == 0) {
        of /= 2;
    }
    if (fundamentals[lvl].find(of) != fundamentals[lvl].end()) {
        return fundamentals[lvl][of];
    } else {
        return unordered_set<uint64_t>{};
    }
}

/*
    Filling 0 cost vector -> 2^i
*/
vector<uint64_t> get0cost(size_t bits) {
    vector<uint64_t> cost0(bits);
    for (uint64_t i = 0; i < bits; i++) {
        cost0[i] = (1ll << i);
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
        if (status_of_new_int(cost0[i] - 1, 1) == 2) {
            cost1.push_back(cost0[i] - 1);
            already_found.insert({cost0[i] - 1, 1});
        }

        if (status_of_new_int(cost0[i] + 1, 1) == 2) {
            cost1.push_back(cost0[i] + 1);
            already_found.insert({cost0[i] + 1, 1});
        }
    }
    cost1.shrink_to_fit();

    return std::move(cost1);
}


void concider_integer_cost(uint64_t consider, vector<uint64_t> maybe_fundamental, vector<uint64_t>& int_vector, uint64_t cost, bool debug = false) {
    auto was_consider = consider;
    if (consider == 0) {
        return;
    }

    while(consider % 2 == 0) {
        consider /= 2;
    }

    if (consider == 889 && debug) {
        cout << "DEBUG: " << was_consider << endl;
    }

    if (uint64_t status = status_of_new_int(consider, cost)) {
        if (status == 2) {
            int_vector.push_back(consider);
            already_found.insert({consider, cost});
        }
        for (auto fundamental : maybe_fundamental) {
            add_fundamental_to(fundamental, consider, cost);
        }
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
            bool debug = (cost1[j] == 31);
            concider_integer_cost(cost0[i] * cost1[j] - 1, {cost1[j]}, cost2, 2, debug);
            concider_integer_cost(cost0[i] * cost1[j] + 1, {cost1[j]}, cost2, 2, debug);
            concider_integer_cost(cost0[i] + cost1[j], {cost1[j]}, cost2, 2, debug);
            concider_integer_cost(cost0[i] - cost1[j], {cost1[j]}, cost2, 2, debug);
            concider_integer_cost(-cost0[i] + cost1[j], {cost1[j]}, cost2, 2, debug);
            //concider_integer_cost(-cost0[i] - cost1[j], {cost1[j]}, cost2, 2); // should never be added as it is < 0
            concider_integer_cost(cost0[i] * cost1[j] - cost1[j], {cost1[j]}, cost2, 2, debug);
            concider_integer_cost(cost0[i] * cost1[j] + cost1[j], {cost1[j]}, cost2, 2, debug);
        }
    }
    cost2.shrink_to_fit();

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

    cout << "Counting combination of cost2 + fundamental" << endl;
    // combination fo cost2 + fundamental
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost2size; j++) {
            const unordered_set<uint64_t> fundamentals_set = get_fundamentals(cost2[j], 2);
            for (auto fundamental : fundamentals_set) {
                concider_integer_cost(cost0[i] * cost2[j] - fundamental, {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * cost2[j] + fundamental, {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * fundamental - cost2[j], {cost2[j], fundamental}, cost3, 3);
                concider_integer_cost(cost0[i] * fundamental + cost2[j], {cost2[j], fundamental}, cost3, 3);
            }
        }
    }

    cout << "Counting combination of two cost1" << endl;
    // combination of two cost1
    for (uint64_t i = 0; i < cost1size; i++) {
        for (uint64_t j = 0; j < cost1size; j++) {
            if (cost1[i] == cost1[j]) {
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

    cout << "Counting combination of cost2 + cost0 and cost2 + cost2" << endl;
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

    cost3.shrink_to_fit();

    return std::move(cost3);
}


void concider_terminating(uint64_t consider, vector<uint64_t>& consider_terminating) {
    if (consider == 0) {
        return;
    }

    while(consider % 2 == 0) {
        consider /= 2;
    }

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
    const size_t cost0size = cost0.size();
    const size_t cost1size = cost1.size();
    const size_t cost2size = cost2.size();
    const size_t cost3size = cost3.size();
    vector<uint64_t> cost4;
    cost4.reserve(cost1size * cost1size * cost0size * 8);

    cout << "Counting combination of cost3 + fundamental" << endl;
    // combination fo cost3 + fundamental
    for (uint64_t i = 0; i < cost0size; i++) {
        for (uint64_t j = 0; j < cost3size; j++) {
            const unordered_set<uint64_t> fundamentals_set = get_fundamentals(cost3[j], 3);
            for (auto fundamental : fundamentals_set) {
                concider_integer_cost(cost0[i] * cost3[j] - fundamental, {}, cost4, 4);
                concider_integer_cost(cost0[i] * cost3[j] + fundamental, {}, cost4, 4);
                concider_integer_cost(cost0[i] * fundamental - cost3[j], {}, cost4, 4);
                concider_integer_cost(cost0[i] * fundamental + cost3[j], {}, cost4, 4);
            }

        }
    }

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

    cout << "Counting combination of cost3 + cost0 and cost3 + cost3" << endl;
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

    return std::move(cost4);
}


int main() {

    vector<uint64_t> cost0 = get0cost(20);
    cout << "cost0 filled " << cost0.size() << endl;
    /*set<uint64_t> cost00;
    for (auto x : cost0) {
        cost00.insert(x);
    }
    for (auto x : cost00) {
        cout << x << ' ';
    }
    cout << endl; */

    vector<uint64_t> cost1 = get1cost(cost0);
    cout << "cost1 filled " << cost1.size() << endl;
    /*set<uint64_t> cost11;
    for (auto x : cost1) {
        cost11.insert(x);
    }
    for (auto x : cost11) {
        cout << x << ' ';
    }
    cout << endl;*/

    vector<uint64_t> cost2 = get2cost(cost0, cost1);
    cout << "cost2 filled " << cost2.size() << endl;
    /*set<uint64_t> cost22;
    for (auto x : cost2) {
        cost22.insert(x);
    }
    for (auto x : cost22) {
        cout << x << ' ';
    }
    cout << endl;*/

    vector<uint64_t> cost3 = get3cost(cost0, cost1, cost2);
    cout << "cost3 filled " << cost3.size() << endl;
    /*set<uint64_t> cost33;
    for (auto x : cost3) {
        cost33.insert(x);
    }
    for (auto x : cost33) {
        cout << x << ' ';
    }
    cout << endl; */

    vector<uint64_t> cost4 = get4cost(cost0, cost1, cost2, cost3);
   /* cout << "cost4 filled " << cost4.size() << endl;
    set<uint64_t> cost44;
    for (auto x : cost4) {
        cost44.insert(x);
    }
    for (auto x : cost44) {
        cout << x << ' ';
    }
    cout << endl; */

    cout << already_found.size() << endl;
    vector<pair<uint64_t,uint64_t> > ans;
    ans.reserve(already_found.size());
    for (auto x : already_found) {
        ans.push_back(x);
    }
    sort(ans.begin(), ans.end());
    int now = 3;
    for (int i = 0; i < ans.size(); i++) {
        if (ans[i].second == 0) {
            continue;
        }
        if (ans[i].first != now) {
            cout << "First number is: " << now << endl;
            break;
        }
        now += 2;
    }
    for (uint64_t i = 0; i < ans.size(); i++) {
        if (ans[i].second == 0 && ans[i].first != 1) {
            continue;
        }
        cout << ans[i].first << ' ' << ans[i].second << endl;
    }

}