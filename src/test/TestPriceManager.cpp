// #define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "PriceManagerModule"

#include <boost/test/included/unit_test.hpp>
#include "model/PriceManager.h"
#include <iostream>

using namespace std;

BOOST_AUTO_TEST_CASE(test1)
{
  cout << "Begin" << endl;
  shared_ptr<Price> out;
  BOOST_CHECK(!out);
  {
    cout << "inner group begin" << endl;
    shared_ptr<Price> p = PriceManager::GetInstance()->Allocate();
    out = p;
    shared_ptr<Price> p2 = PriceManager::GetInstance()->Allocate();
    cout << "inner group end" << endl;
  }
  cout << "After inner gourp" << endl;
  BOOST_CHECK(out);
}
