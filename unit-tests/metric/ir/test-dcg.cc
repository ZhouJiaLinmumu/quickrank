#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "metric/ir/dcg.h"
#include <cmath>

BOOST_AUTO_TEST_CASE( dcg_test )
{
  double labels [] = {3,2,1,0,0};
  ResultList results (5, &labels[0], 666);

  quickrank::metric::ir::Dcg dcg_metric(5);

  // DCG@k computation with K > num results
  BOOST_CHECK_EQUAL( dcg_metric.evaluate_result_list(results) ,
      ( (pow(2,labels[0])-1) + (pow(2,labels[1])-1)/log2(3) + (pow(2,labels[2])-1)/2
      )
  );

  // DCG@k computation with K < num results
  dcg_metric.set_cutoff(2);
  BOOST_CHECK_EQUAL( dcg_metric.evaluate_result_list(results) ,
      ( (pow(2,labels[0])-1) + (pow(2,labels[1])-1)/log2(3)
      )
  );

  // DCG@k computation with K = 0
  dcg_metric.set_cutoff(0);
  BOOST_CHECK_EQUAL( dcg_metric.evaluate_result_list(results) ,
      ( (pow(2,labels[0])-1) + (pow(2,labels[1])-1)/log2(3) + (pow(2,labels[2])-1)/2
      )
  );

  // DCG@k computation with K = No cutoff
  dcg_metric.set_cutoff(dcg_metric.NO_CUTOFF);
  BOOST_CHECK_EQUAL( dcg_metric.evaluate_result_list(results) ,
      ( (pow(2,labels[0])-1) + (pow(2,labels[1])-1)/log2(3) + (pow(2,labels[2])-1)/2
      )
  );
}