#include "json.hpp"
using json = nlohmann::json;
#include <vector>
#include <iostream>
#include <map>
using namespace std;

void func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "li si";
    js["msg"] = "Hello,world!";
    cout << js << endl;
}
void func2()
{
    json js;
    vector<int> vec{1, 2, 3, 4};
    js["vec"] = vec;
    map<string, int> mp;
    mp.insert({"黄山", 1});
    mp.insert({"华山", 2});
    js["map"] = mp;
    // string s = js.dump();
    cout << js << endl;
}
int main()
{
    func2();
    return 0;
}