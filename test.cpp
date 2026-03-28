#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <climits>
#include <queue>
#include <cstdlib>
#include <iostream>

using namespace std;

vector<string> split(const string& s) { //разбивает строку
    vector<string> res;
    string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == ' ' || s[i] == '\n') {
            res.push_back(cur);
            cur.clear();
        } else {
            cur += s[i];
        }
    }
    if (!cur.empty()){
        res.push_back(cur);
    } 
    return res;
}

struct Interval { // структура для интервала
    int l, r;
    Interval(int l = 1, int r = 0) : l(l), r(r) {}
    bool valid() const { 
        return l <= r; 
    }
};

Interval intersection(const Interval& a, const Interval& b) {
    return Interval(max(a.l, b.l), min(a.r, b.r));
}

Interval merge(const Interval& a, const Interval& b) {
    if (!a.valid()) return b;
    if (!b.valid()) return a;
    return Interval(min(a.l, b.l), max(a.r, b.r));
}

Interval add(const Interval& a, const Interval& b) {
    return Interval(a.l + b.l, a.r + b.r);
}

Interval sub(const Interval& a, const Interval& b) {
    return Interval(a.l - b.r, a.r - b.l);
}

Interval mul(const Interval& a, const Interval& b) {
    if (!a.valid() || !b.valid()) return Interval();
    int v1 = a.l * b.l, v2 = a.l * b.r, v3 = a.r * b.l, v4 = a.r * b.r;
    return Interval(min(min(v1, v2), min(v3, v4)), max(max(v1, v2), max(v3, v4)));
}

using State = map<string, Interval>;

Interval get_inter(const State& st, const string& s){ //получает интервал для регистра или числа
    if (s.empty()) return Interval();
    if (isdigit(s[0]) || (s[0] == '-' && isdigit(s[1]))) {
        int x = stoi(s);
        return Interval(x, x);
    }
    auto pos = st.find(s); //ищем регистр
    return pos != st.end() ? pos->second : Interval();
}

struct Node {
    string op, a, b, c;
    vector<int> next;
    int line;
};

State execute(const Node& node, const State& st) { //выполняет операции
    State st2 = st;

    if (node.op == "add")
        st2[node.c] = add(get_inter(st, node.a), get_inter(st, node.b));
    else if (node.op == "sub")
        st2[node.c] = sub(get_inter(st, node.a), get_inter(st, node.b));
    else if (node.op == "mul")
        st2[node.c] = mul(get_inter(st, node.a), get_inter(st, node.b));
    else if (node.op == "str")
        st2[node.b] = get_inter(st, node.a);

    return st2;
}

vector<Node> parse(const string& fname) { //делает из файла граф
    FILE* f = fopen(fname.c_str(), "r");
    
    vector<pair<int, string> > lines;
    char buf[1024];
    int line_num = 0;
    map<int, int> ltoi; //соответствие между строками (нумерацией) файла и индексами узлов
    vector<Node> nodes;

    while (fgets(buf, sizeof(buf), f)) {
        line_num++;
        string line(buf);
        
        // Проверяем, начинается ли строка с комментария
        if (line[0] == '#') {
            continue;
        }

        vector<string> tokens = split(line);
        
        Node node;
        node.line = line_num;
        node.op = tokens[0];
        
        //заполняем токены согласно тому, какую операцию рассматриваем
        if (node.op == "str") {
            node.a = tokens[1];
            node.b = tokens[2];
        }
        else if (node.op == "add" || node.op == "sub" || node.op == "mul") {
            node.a = tokens[1];
            node.b = tokens[2];
            node.c = tokens[3];
        }
        else if (node.op == "jmp") {
            node.next.push_back(stoi(tokens[1]));
        }
        else if (node.op[0] == 'j') {
            node.a = tokens[1];
            node.b = tokens[2];
            node.next.push_back(stoi(tokens[3]));
        }
        
        ltoi[line_num] = nodes.size(); //строим соответствие
        nodes.push_back(node);
    }
    
    fclose(f);
    int max_line = line_num;
    
    //проходимся по nodes, номера строк меняем на индексы узлов, добавляем индексы для условных переходов
    for (size_t i=0; i < nodes.size(); i++){
        if (nodes[i].op[0] == 'j'){
            vector<int> new_next; //для новых переходов

            for (size_t j = 0; j < nodes[i].next.size(); j++){
                int ln = nodes[i].next[j]; //номер строки
                if (ltoi.find(ln) != ltoi.end()){
                    int indx = ltoi[ln]; //индекс узла
                    new_next.push_back(indx);
                }
                
            }

            if (nodes[i].op != "jmp") {
                int nextln = nodes[i].line + 1;

                while (nextln <= max_line){
                    if (ltoi.find(nextln) != ltoi.end()){
                        int nextindx = ltoi[nextln];
                        new_next.push_back (nextindx);
                        break;
                    }
                    nextln++;
                }
            }
            nodes[i].next = new_next;            
        } else if (i + 1 < nodes.size()) {
                nodes[i].next.push_back(i + 1);
        }
    }
    return nodes;
}


void dfs (const vector<Node>& graph, int v, vector<int>& visited, vector<int>& order){ //обход графа в глубину
    visited[v] = 1;
    for(size_t i = 0; i < graph[v].next.size(); i++){
        int ver = graph[v].next[i];
        if (visited[ver] == 0){
            dfs (graph, ver, visited, order);
        }
    }
    order.push_back(v);
}

vector<int> topo(const vector<Node>& graph) { //топологическая сортировка
    vector<int> visited(graph.size(), 0);
    vector<int> order;

    for (size_t i=0; i < graph.size(); i++){
        if (visited[i] == 0){
            dfs (graph, i, visited, order);
        }
    }
    reverse (order.begin(), order.end());
    return order;
}

Interval analyze(Interval x, Interval y, const vector<Node>& nodes) { //анализ интервалов
    size_t n = nodes.size();
    vector<State> IN(n), OUT(n);
    IN[0]["arg0"] = x;
    IN[0]["arg1"] = y;
    vector<int> order = topo(nodes);

    for (size_t i = 0; i < order.size(); i++){ //проходим по вершинам графа в порядке очереди 
        int v = order[i];
        State in = IN[v];

        string oper = nodes[v].op;
        if (oper == "jg" || oper == "jge" ||oper == "jl" || oper == "jle"){
            string reg = nodes[v].a;
            int val = stoi(nodes[v].b);

            Interval t, f; //интервалы для разветвления на правду и ложь
            State stt = in, stf = in; //состояния для разветвления

            auto it = in.find(reg);
            if (it == in.end()) continue;
            //определяем интервалы для двух веток:
            if (oper == "jg") {
                t = Interval(val + 1, INT_MAX);
                f = Interval(INT_MIN, val);
            } else if (oper == "jge") {
                t = Interval(val, INT_MAX);
                f = Interval(INT_MIN, val - 1);
            } else if (oper == "jl") {
                t = Interval(INT_MIN, val - 1);
                f = Interval(val, INT_MAX);
            } else if (oper == "jle") {
                t = Interval(INT_MIN, val);
                f = Interval(val + 1, INT_MAX);
            }
            
            //пересекаем с интервалами веток, чтобы получить ограничение на регистры
            stt[reg] = intersection(it->second, t);
            stf[reg] = intersection(it->second, f);

            if (stt[reg].valid()) {
                int u = nodes[v].next[0];
                for (auto it = stt.begin(); it != stt.end(); ++it)
                    IN[u][it->first] = merge(IN[u][it->first], it->second);
            }

            if (stf[reg].valid()) {
                int u = nodes[v].next[1];
                for (auto it = stf.begin(); it != stf.end(); ++it)
                    IN[u][it->first] = merge(IN[u][it->first], it->second);
            }

            continue;
        }

        OUT[v] = (oper == "nop") ? in : execute(nodes[v], in); //выполняем операции

        for (size_t i = 0; i < nodes[v].next.size(); i++) {
            int u = nodes[v].next[i];
            for (auto it = OUT[v].begin(); it != OUT[v].end(); ++it)
                IN[u][it->first] = merge(IN[u][it->first], it->second);
        }
    }

    Interval res;
    for (size_t v = 0; v < n; v++) {
        if (nodes[v].next.empty()) {
            auto it = OUT[v].find("ret");
            if (it != OUT[v].end()) {
                res = merge(res, it->second);
            }
        }
    }
    return res;
}

int main() {
    vector<Node> nodes = parse("testcase_mul.reil");

    vector<pair<Interval,Interval>> tests = {{{1,3},{2,4}},{{4,5},{2,4}},{{2,4},{2,4}}};

    for (size_t i = 0; i < tests.size(); i++) {
        Interval x = tests[i].first;
        Interval y = tests[i].second;
        Interval r = analyze(x, y, nodes);
        cout << "x=[" << x.l << "," << x.r << "], y=[" << y.l << "," << y.r
             << "] -> ret=[" << r.l << "," << r.r << "]\n";
    }

    return 0;
}