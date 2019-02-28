#define BOOST_TEST_MODULE "OrderManagerTest"
#include <boost/test/included/unit_test.hpp>

#include "model/OrderManager.h"
#include <iostream>
#include <vector>

using namespace std;

BOOST_AUTO_TEST_CASE(testOrderManager)
{
  vector<OrderPtr> orders;
  auto ord1 = Message<Order>::New();
  ord1->status = Proto::OrderStatus::New;
  BOOST_CHECK(ord1);
  orders.push_back(ord1);
  auto ord2 = Message<Order>::New();
  ord2->status = Proto::OrderStatus::Filled;
  BOOST_CHECK(ord2);
  orders.push_back(ord2);
  OrderManager::GetInstance()->OnOrder(orders);

  auto ord11 = OrderManager::GetInstance()->FindOrder(ord1->id);
  BOOST_CHECK(ord11);
  BOOST_CHECK_EQUAL(ord1, ord11);

  auto ord12 = OrderManager::GetInstance()->FindActiveOrder(ord1->id);
  BOOST_CHECK(ord12);
  BOOST_CHECK_EQUAL(ord1, ord12);

  auto ord13 = OrderManager::GetInstance()->FindInactiveOrder(ord1->id);
  BOOST_CHECK(!ord13);

  auto ord21 = OrderManager::GetInstance()->FindOrder(ord2->id);
  BOOST_CHECK(ord21);
  BOOST_CHECK_EQUAL(ord2, ord21);

  auto ord22 = OrderManager::GetInstance()->FindActiveOrder(ord2->id);
  BOOST_CHECK(!ord22);

  auto ord23 = OrderManager::GetInstance()->FindInactiveOrder(ord2->id);
  BOOST_CHECK(ord23);
  BOOST_CHECK_EQUAL(ord2, ord23);

  auto ord24 = Message<Order>::New(ord2);
  BOOST_CHECK(ord2 != ord24);
  BOOST_CHECK_EQUAL(ord2->id, ord24->id);
  BOOST_CHECK(ord2->status == ord24->status);
  BOOST_CHECK_EQUAL(ord2->header.time, ord24->header.time);

  auto ord = OrderManager::GetInstance()->FindOrder(23);
  BOOST_CHECK(!ord);
}
