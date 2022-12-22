//
//  main.cpp
//  zmz_shared_ptr
//
//  Created by mingze.zou on 2022/11/10.
//

#include <iostream>
//#include <memory>
#include <string>
#include "shared_ptr.h"

using namespace std;

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    zmz_unique_ptr<string> p1(new string("adsadadsad"));
    zmz_shared_ptr<string> sp2(new string("Lady Gaga"));
    
    return 0;
}
