#include <iostream>
#include <typeinfo>
#include <any>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <cctype>
#include <cstring>

using namespace std;

struct JsonArray : public vector<any>
{
    JsonArray() {}

    JsonArray(const vector<any>& arr) {
        this->clear();
        std::copy(arr.begin(), arr.end(), std::back_inserter(*this));
    }

    JsonArray &operator=(const vector<any> &arr)
    {
        this->clear();
        std::copy(arr.begin(), arr.end(), std::back_inserter(*this));
        return *this;
    }

    static const type_info &type() { return typeid(vector<any>); }
};

struct JsonObject : public map<string,any>
{
    JsonObject() {}

    JsonObject(const map<string,any>& map) {
        this->clear();
        this->insert(map.begin(), map.end());
    }

    JsonObject &operator=(const map<string, any> &map)
    {
        this->clear();
        this->insert(map.begin(), map.end());
        return *this;
    }

    static const type_info &type() { return typeid(map<string,any>); }
};

struct JsonString : public string
{
    static const type_info &type() { return typeid(string); }

    static string escape(const std::string &str)
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

};

struct JsonInt
{
    static const type_info &type() { return typeid(int); }
};

struct JsonFloat
{
    static const type_info &type() { return typeid(float); }
};

struct JsonBool
{
    static const type_info &type() { return typeid(bool); }
};

struct JsonNull
{
    static const type_info &type() { return typeid(nullptr); }
};

ostream &operator<<(ostream&, JsonArray&);
ostream &operator<<(ostream&, JsonObject&);

ostream &operator<<(ostream &os, JsonObject &obj)
{
    size_t count = 0;

    os << "{";
    for (auto &[key, val] : obj)
    {
        if (count) os << ",";
        os << '"' << key << '"' << ':';

        if (val.type() == JsonInt::type())
        {
            os << any_cast<int>(val);
        }
        else if (val.type() == JsonFloat::type())
        {
            os << any_cast<float>(val);
        }
        else if (val.type() == JsonBool::type())
        {
            bool tmp = any_cast<bool>(val);
            if (tmp) os << "true";
            else os << "false";
        }
        else if (val.type() == JsonString::type())
        {
            os << '"' << JsonString::escape(any_cast<string>(val)) << '"';
        }
        else if (val.type() == JsonArray::type())
        {
            JsonArray jarr(any_cast<vector<any>&>(val));
            os << jarr;
        }
        else if (val.type() == typeid(JsonArray))
        {
            os << any_cast<JsonArray&>(val);
        }
        else if (val.type() == JsonObject::type())
        {
            JsonObject jobj(any_cast<map<string, any>&>(val));
            os << jobj;
        }
        else if (val.type() == typeid(JsonObject))
        {
            os << any_cast<JsonObject&>(val);
        }
        else if (val.has_value() && val.type() == JsonNull::type()) {
            os << "null";
        }
        else
        {
            cerr << "bad type: " << val.type().name() << endl;
            throw runtime_error("Invalid JSON object");
        }

        count++;
    }

    os << "}";
    return os;
}

ostream &operator<<(ostream &os, JsonArray &arr)
{
    size_t count = 0;

    os << "[";
    for (auto &item : arr)
    {
        if (count) os << ",";

        if (item.type() == JsonInt::type())
        {
            os << any_cast<int>(item);
        }
        else if (item.type() == JsonFloat::type())
        {
            os << any_cast<float>(item);
        }
        else if (item.type() == JsonBool::type())
        {
            bool tmp = any_cast<bool>(item);
            if (tmp) os << "true";
            else os << "false";
        }
        else if (item.type() == JsonString::type())
        {
            os << '"' << JsonString::escape(any_cast<string>(item)) << '"';
        }
        else if (item.type() == JsonArray::type())
        {
            JsonArray jarr(any_cast<vector<any>&>(item));
            os << jarr;
        }
        else if (item.type() == typeid(JsonArray))
        {
            os << any_cast<JsonArray&>(item);
        }
        else if (item.type() == JsonObject::type())
        {
            JsonObject jobj(any_cast<map<string, any>&>(item));
            os << jobj;
        }
        else if (item.type() == typeid(JsonObject))
        {
            os << any_cast<JsonObject&>(item);
        }
        else if (item.has_value() && item.type() == JsonNull::type())
        {
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

istream &operator>>(istream&, JsonObject&);
istream &operator>>(istream&, JsonArray&);

istream &operator>>(istream &is, JsonObject &obj)
{
    char ch;
    is >> ch;

    if (ch != '{') throw runtime_error("Invalid JSON Object");

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
            if (ch == '"') break;
            throw runtime_error("Invalid JSON key sequence: " + ch);
        }

        if (ch != '"') {
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
            JsonArray arr;
            is >> arr;
            obj[key] = arr;
            continue;
        }
        
        // OBJECT
        if(ch == '{') {
            is.putback(ch);
            JsonObject obj2;
            is >> obj2;
            obj[key] = obj2;
            continue;
        }

        // STRING
        if (ch == '"') {
            while (is.get(ch)) {
                if (ch == '\\') {
                    tok += ch;
                    is.get(ch);
                    tok += ch;
                    continue;
                }
                if (ch == '"') break;
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

    return is;
}

istream &operator>>(istream &is, JsonArray &arr)
{
    char ch;
    is >> ch;

    if (ch != '[') throw runtime_error("Invalid JSON Array");

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
            JsonArray arr2;
            is >> arr2;
            arr.push_back(arr2);
            continue;
        }

        // OBJECT
        if(ch == '{') {
            is.putback(ch);
            JsonObject obj;
            is >> obj;
            arr.push_back(obj);
            continue;
        }

        // STRING
        if (ch == '"') {
            while (is.get(ch)) {
                if (ch == '\\') {
                    tok += ch;
                    is.get(ch);
                    tok += ch;
                    continue;
                }
                if (ch == '"') break;
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

    return is;
}

int main(int argc, char *argv[])
{
    map<string, any> m;
    m["test1"] = 1;
    m["test2"] = string("hello\" \\ \x55 \'");
    m["test3"] = 3.14f;
    m["test4"] = true;
    m["test5"] = false;
    m["test5"] = nullptr;

    JsonObject jm1(m);
    cout << "JM1:" << jm1 << endl;

    vector<any> v;
    v.push_back(123);
    v.push_back(string("hello vec\n"));
    v.push_back(m);
    m["test7"] = 999;

    JsonArray ja1(v);
    cout << "JA1:" << ja1 << endl;

    m["vec"] = v;
    JsonObject jm2(m);
    cout << "JM2:" << jm2 << endl;

    JsonObject jm3;
    //string jsonString = R"({  "name":"John \"Smith", "age": 30, "isStudent": true, "scores": [90, 85, 95]})";
    //string jsonString = string("{ \n \"nullval\": null ,\n \"name\":\"Joh\042n\t \\\"Sm\x22ith\", \"age\": 30.81 , \"isStudent\": true, \"scores\"  : [ { \"k\":\"v\" }, [ 210 ] , 90 , 85, 95.7]}");
    string jsonString = string("{ \n \"nullval\": null ,\n \"name\":\"John\t \\\"Smith\", \"age\": 30.81 , \"isStudent\": true, \"scores\"  : [ { \"k\":\"v\" }, [ 210 ] , 90 , 85, 95.7]}");
    cout << "Parsing JSON:\n" << jsonString << endl;
    stringstream ss(jsonString);
    ss >> jm3;
    cout << "GRAND FINALE:" << jm3 << endl;

    return 0;
}
