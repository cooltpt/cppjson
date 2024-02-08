#include <iostream>
#include <typeinfo>
#include <any>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <regex>
#include <cctype>
#include <cstring>

using namespace std;

const type_info &vectorType = typeid(vector<any>);
const type_info &mapType    = typeid(map<string, any>);
const type_info &stringType = typeid(string);
const type_info &intType    = typeid(int);
const type_info &floatType  = typeid(float);
const type_info &boolType   = typeid(bool);

#if 0
struct JsonArray : public vector<any>
{
    const type_info &type() const { return typeid(vector<any>); }
};

struct JsonMap : public map<string,any>
{
    const type_info &type() const { return typeid(map<string,any>); }
};
#endif

struct JSON
{
    any value;
    string type_name;

    JSON() {}

    JSON(const JSON& json) {
        this->value = json.value;
        this->type_name = json.type_name;
    }

    JSON(const vector<any> &_value)
    {
        this->value = any_cast<vector<any>>(_value);
        this->type_name = vectorType.name();
    }

    JSON(const map<string, any> &_value)
    {
        this->value = any_cast<map<string, any>>(_value);
        this->type_name = mapType.name();
    }

    bool isVector() { return (type_name == vectorType.name()); }
    bool isMap() { return (type_name == mapType.name()); }
    bool isString() { return (type_name == stringType.name()); }
    bool isInt() { return (type_name == intType.name()); }
    bool isFloat() { return (type_name == floatType.name()); }
    bool isBool() { return (type_name == boolType.name()); }

    static string escapeString(const std::string &str)
    {
        std::string s = "";

        for (char c : str)
        {
            if (' ' <= c && c <= '~' && c != '\\' && c != '"')
            {
                s += c;
            }
            else
            {
                s += '\\';
                switch (c)
                {
                case '"':
                    s += '"';
                    break;
                case '\\':
                    s += '\\';
                    break;
                case '\t':
                    s += 't';
                    break;
                case '\r':
                    s += 'r';
                    break;
                case '\n':
                    s += 'n';
                    break;
                default:
                    char const *const hexdig = "0123456789ABCDEF";
                    s += 'x';
                    s += hexdig[c >> 4];
                    s += hexdig[c & 0xF];
                }
            }
        }

        return s;
    }

    JSON &operator=(const map<string, any> &_value)
    {
        this->value = any_cast<map<string, any>>(_value);
        this->type_name = mapType.name();
        return *this;
    }

    JSON &operator=(const vector<any> &_value)
    {
        this->value = any_cast<vector<any>>(_value);
        this->type_name = vectorType.name();
        return *this;
    }

    JSON &operator=(const string &_value)
    {
        this->value = any_cast<string>(_value);
        this->type_name = stringType.name();
        return *this;
    }

    JSON &operator=(const float _value)
    {
        this->value = any_cast<float>(_value);
        this->type_name = floatType.name();
        return *this;
    }

    JSON &operator=(const int _value)
    {
        this->value = any_cast<int>(_value);
        this->type_name = intType.name();
        return *this;
    }
};

istream &operator>>(istream &is, JSON &j)
{
    char ch;
    is >> ch;

    // OBJECT
    if (ch == '{') {
        map<string,any> obj;

        while (1)
        {
            string tok;
            string key;

            is >> ch;
            if (ch == '}') break;
            if (ch == ',') continue;
            is.putback(ch);

            //
            // KEY
            //
            while (is.get(ch)) {
                if (isspace(ch)) continue;
                if (iscntrl(ch)) continue;
                if (ch == '\"') break;
                throw runtime_error("Invalid JSON key sequence: " + ch);
            }

            if (ch != '\"') {
                throw runtime_error("Invalid JSON key sequence: " + ch);
            }

            while (is.get(ch) && ch != '"') key += ch;
            while (is.get(ch) && ch != ':');

            //
            // VALUE
            //
            is >> ch;
            
            // ARRAY
            if(ch == '[') {
                is.putback(ch);
                JSON arr;
                is >> arr;
                obj[key] = any_cast<vector<any>>(arr.value);
                continue;
            }
            
            // OBJECT
            if(ch == '{') {
                is.putback(ch);
                JSON obj2;
                is >> obj2;
                obj[key] = any_cast<map<string,any>>(obj2.value);
                continue;
            }

            // STRING
            if (ch == '\"') {
                while (is.get(ch)) {
                    if (ch == '\\') {
                        tok += ch;
                        is.get(ch);
                        tok += ch;
                        continue;
                    }
                    if (ch == '\"') break;
                    tok += ch;
                }

                obj[key] = any_cast<string>(tok);
                continue;
            }
            
            // NUMERIC/BOOL/NULL
            if (isdigit(ch) || ch == '+' || ch == '-') {
                is.putback(ch);
            } else if (ch == 't' || ch == 'f') {
                is.putback(ch);
            } else if (ch == 'n') {
                is.putback(ch);
            } else {
                //noop
            }

            while(is.get(ch)) {
                if (isspace(ch)) break;
                if (ch == ',') break;
                if (ch == '}') {
                    is.putback(ch);
                    break;
                }
                tok += ch;
            }

            if (tok == "true") {
                obj[key] = any_cast<bool>(true);
                continue;
            }
            if (tok == "false") {
                obj[key] = any_cast<bool>(false);
                continue;
            }
            if (tok == "null") {
                obj[key] = nullptr;
                continue;
            }

            if (regex_match(tok, regex("[(-|+)|][\\.0-9]+"))) {
                if (strchr(tok.c_str(),'.')) {
                    obj[key] = any_cast<float>(strtof32(tok.c_str(),0));
                } else {
                    obj[key] = any_cast<int>(atoi(tok.c_str()));
                }
                continue;
            }

            // if we made it here, something is wrong
            throw runtime_error("Invalid JSON token: " + tok);

        } // while

        j = obj;
        return is;
    } // OBJECT

    // ARRAY
    if (ch == '[') {
        vector<any> arr;

        while (1)
        {
            string tok;

            is >> ch;
            if (ch == ']') break;
            if (ch == ',') continue;

            // ARRAY
            if (ch == '[')
            {
                is.putback(ch);
                JSON arr2;
                is >> arr2;
                arr.push_back(any_cast<vector<any>>(arr2.value));
                continue;
            }

            // OBJECT
            if(ch == '{') {
                is.putback(ch);
                JSON obj2;
                is >> obj2;
                arr.push_back(any_cast<map<string,any>>(obj2.value));
                continue;
            }

            // STRING
            if (ch == '\"') {
                while (is.get(ch)) {
                    if (ch == '\\') {
                        tok += ch;
                        is.get(ch);
                        tok += ch;
                        continue;
                    }
                    if (ch == '\"') break;
                    tok += ch;
                }

                arr.push_back(any_cast<string>(tok));
                continue;
            }

            // NUMERIC/BOOL/NULL
            if (isdigit(ch) || ch == '+' || ch == '-') {
                is.putback(ch);
            } else if (ch == 't' || ch == 'f') {
                is.putback(ch);
            } else if (ch == 'n') {
                is.putback(ch);
            } else {
                //noop
            }

            while(is.get(ch)) {
                if (isspace(ch)) break;
                if (ch == ',') break;

                if (ch == ']') {
                    is.putback(ch);
                    break;
                }
                tok += ch;
            }

            if (tok == "true") {
                arr.push_back(any_cast<bool>(true));
                continue;
            }
            if (tok == "false") {
                arr.push_back(any_cast<bool>(false));
                continue;
            }
            if (tok == "null") {
                arr.push_back(nullptr);
                continue;
            }

            if (regex_match(tok, regex("[(-|+)|][\\.0-9]+"))) {
                if (strchr(tok.c_str(),'.')) {
                    arr.push_back(any_cast<float>(strtof32(tok.c_str(),0)));
                } else {
                    arr.push_back(any_cast<int>(atoi(tok.c_str())));
                }
                continue;
            }

            // if we made it here, something is wrong
            throw runtime_error("Invalid JSON token: " + tok);

        } // while

        j = arr;
        return is;
    } // ARRAY

    return is;
}

ostream &operator<<(ostream &os, JSON &j)
{
    size_t count = 0;

    // STRING
    if (j.isString())
    {
        os << '"' << JSON::escapeString(any_cast<string>(j.value)) << '"';
        return os;
    }

    // NUMBER (float,int,...)
    if (j.isInt())
    {
        os << any_cast<int>(j.value);
        return os;
    }
    if (j.isFloat())
    {
        os << any_cast<float>(j.value);
        return os;
    }

    // BOOL
    if (j.isBool())
    {
        bool tmp = any_cast<bool>(j.value);
        if (tmp)
            os << "true";
        else
            os << "false";
        return os;
    }

    // ARRAY vector<any>
    if (j.isVector())
    {
        os << "[";
        for (auto &item : any_cast<vector<any>>(j.value))
        {
            if (count)
                os << ",";

            if (item.type() == intType) // NUMERIC
            {
                os << any_cast<int>(item);
            }
            else if (item.type() == floatType) // NUMERIC
            {
                os << any_cast<float>(item);
            }
            else if (item.type() == boolType) // BOOL
            {
                bool tmp = any_cast<bool>(item);
                if (tmp) os << "true";
                else os << "false";
            }
            else if (item.type() == stringType) // STRING
            {
                os << '\"' << JSON::escapeString(any_cast<string>(item)) << '\"';
            }
            else if (item.type() == vectorType) // ARRAY
            {
                JSON js = any_cast<vector<any>>(item);
                os << js;
            }
            else if (item.type() == mapType) // OBJECT
            {
                JSON js = any_cast<map<string, any>>(item);
                os << js;
            }
            else if (item.has_value() && item.type() == typeid(nullptr)) {
                os << "null";
            }
            else
            {
                cerr << "bad type: " << item.type().name() << endl;
                throw runtime_error("Invalid JSON object");
            }

            count++;
        }
        os << "]";
        return os;
    }

    // OBJECT map<string,any>
    if (j.isMap())
    {
        os << "{";
        for (auto &[key, val] : any_cast<map<string, any>>(j.value))
        {
            if (count) os << ",";
            os << "\"" << key << "\":";

            if (val.type() == intType) // NUMERIC
            {
                os << any_cast<int>(val);
            }
            else if (val.type() == floatType) //NUMERIC
            {
                os << any_cast<float>(val);
            }
            else if (val.type() == boolType) // BOOL
            {
                bool tmp = any_cast<bool>(val);
                if (tmp) os << "true";
                else os << "false";
            }
            else if (val.type() == stringType) // STRING
            {
                os << '\"' << JSON::escapeString(any_cast<string>(val)) << '\"';
            }
            else if (val.type() == vectorType) // ARRAY
            {
                JSON js = any_cast<vector<any>>(val);
                os << js;
            }
            else if (val.type() == mapType) // OBJECT
            {
                JSON js = any_cast<map<string, any>>(val);
                os << js;
            }
            else if (val.has_value() && val.type() == typeid(nullptr)) {
                os << "null";
            }
            else
            {
                //os << "null";
                cerr << "bad type: " << val.type().name() << endl;
                throw runtime_error("Invalid JSON object");
            }

            count++;
        }
        os << "}";
        return os;
    }

    return os;
}

int main(int argc, char *argv[])
{
    JSON js;

    map<string, any> m;
    m["test1"] = 1;
    m["test2"] = string("hello\" \\ \x55 \'");
    m["test3"] = 3.14f;
    m["test4"] = true;
    m["test5"] = false;

    js = m;
    cout << "jsm:" << js << endl;

    vector<any> v;
    v.push_back(123);
    v.push_back(string("hello vec\n"));
    v.push_back(m);
    m["test6"] = 999;
    js = v;
    cout << "jsv:" << js << endl;

    m["vec"] = v;
    js = m;
    cout << "jsm2:" << js << endl;

    js = v;
    cout << "jsv2:" << js << endl;

    js = string("test (abc\n)\\ \t");

    cout << "isString " << js.isString() << endl;
    cout << js << endl;

    //string jsonString = R"({ \n "name":"John Smith", "age": 30, "isStudent": true, "scores": [90, 85, 95]})";
    string jsonString = string("{ \"nullval\": null ,\n \"name\":\"John\t Smith\", \"age\": 30.81 , \"isStudent\": true, \"scores\"  : [ [ 210 ] , 90 , 85, 95.7]}");
    cout << "Parsing JSON:\n" << jsonString << endl;
    stringstream ss(jsonString);
    ss >> js;

    cout << "js type:" << js.type_name << endl;
    map<string,any> jsm1 = any_cast<map<string,any>>(js.value);
    JSON jsx = jsm1;
    cout << "GRAND FINALE:" << jsx << endl;

    return 0;
}
