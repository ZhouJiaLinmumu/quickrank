#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <iostream>

#include <boost/program_options.hpp>

// TODO: (by cla) Add some logging facility and remove any printf.
// TODO: (by cla) Give names to error codes.
// TODO: (by cla) It seems this log file is now useless ?!?
#ifdef LOGFILE
FILE *flog = NULL;
#endif

#define SHOWTIMER

#include "metric/evaluator.h"
#include "learning/lmart.h"
#include "learning/matrixnet.h"
#include "metric/ir/ndcg.h"
#include "metric/ir/map.h"

namespace po = boost::program_options;

// Structure used to validate allowed metric
struct metric {
	metric(std::string const& val): value(val) { }
	std::string value;
};

// Global function to validate allowed metrics
void validate(boost::any& v,
              std::vector<std::string> const& values,
			  metric* /* target_type */,
              int)
{
	using namespace boost::program_options;

	// Make sure no previous assignment to 'v' was made.
	validators::check_first_occurrence(v);

	// Extract the first string from 'values'. If there is more than
	// one string, it's an error, and exception will be thrown.
	std::string const& s = validators::get_single_string(values);

	if (s == "ndcg" || s == "map") {
		v = boost::any(metric(s));
	} else {
		throw validation_error(validation_error::invalid_option_value);
	}
}

// Auxiliary function to check and set/exit input parameters
template <typename T>
T check_and_set(const po::variables_map &vm, const std::string &name, const std::string &msg)
{
	if (vm.count(name)) {
		return vm[name].as<T>();
	} else {
	    std::cout << msg << std::endl;
	    exit(1);
	}
}


// Auxiliary function to check and set/exit metric
// TODO: smart pointer to be added
qr::metric::ir::Metric* check_and_set_metric(const po::variables_map &vm, const std::string &type)
{
	int k = check_and_set<int>(vm, type + "-cutoff", type + " Metric cutoff was not set.");
	if (vm.count(type + "-metric")) {
			if (vm[type + "-metric"].as<metric>().value == "ndcg")
				return new qr::metric::ir::Ndcg(k);
			else
				return new qr::metric::ir::Map(k);
	} else {
	    std::cout << type + " Metric (ndcg or map) was not set." << std::endl;
		exit(1);
	}
}

int main(int argc, char *argv[]) {

	// Declare the supported options.
	po::options_description model_desc("Model options");
	model_desc.add_options()
		("num-trees,n", po::value<int>(), "set number of trees")
		("shrinkage,s", po::value<float>(), "set shrinkage")
		("num-thresholds,t", po::value<int>(), "set number of thresholds")
		("num-leaves,l", po::value<int>(), "set number of leaves")
		("min-leaf-support,u", po::value<int>(), "set minimum number of leaf support")
		("end-after-rounds,e", po::value<int>(), "set num. rounds with no boost in validation before ending (if 0 disabled")
	;

	po::options_description metric_desc("Metric options");
	metric_desc.add_options()
		("train-metric", po::value<metric>(), "set train metric (allowed values are ndcg and map")
		("train-cutoff", po::value<int>(), "set train metric cutoff")
		("test-metric", po::value<metric>(), "set test metric (allowed values are ndcg and map")
		("test-cutoff", po::value<int>(), "set test metric cutoff")
	;

	po::options_description file_desc("File options");
	file_desc.add_options()
		("partial", po::value<int>(), "set partial file save frequency")
		("train", po::value<std::string>(), "set training file")
		("valid", po::value<std::string>(), "set validation file")
		("test", po::value<std::string>(), "set testing file")
		("features", po::value<std::string>(), "set features file")
		("output", po::value<std::string>(), "set output file")
	;

	po::options_description all_desc("Allowed options");
	all_desc.add(model_desc).add(metric_desc).add(file_desc);
	all_desc.add_options()("help", "produce help message");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, all_desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
	    std::cout << all_desc << "\n";
	    return 1;
	}

	// MODEL STUFF
	unsigned int ntrees         = check_and_set<unsigned int>  (vm, "num-trees", "Number of trees was not set.");
	float        shrinkage      = check_and_set<float>(vm, "shrinkage", "Shrinkage was not set.");
	unsigned int nthresholds    = check_and_set<unsigned int>  (vm, "num-thresholds", "Number of thresholds was not set.");
	unsigned int ntreeleaves    = check_and_set<unsigned int>  (vm, "num-leaves", "Number of leaves was not set.");
	unsigned int minleafsupport = check_and_set<unsigned int>  (vm, "min-leaf-support", "Number of minimum leaf support was not set.");
	unsigned int esr 		    = check_and_set<unsigned int>  (vm, "esr", "Num. rounds with no boost in validation before ending was not set.");

	quickrank::learning::LTR_Algorithm *r = new quickrank::learning::forests::LambdaMart(ntrees, shrinkage, nthresholds, ntreeleaves, minleafsupport, esr);
	//show ranker parameters
	printf("New ranker:\n");
	r->showme();

	// METRIC STUFF
	qr::metric::ir::Metric* training_scorer = check_and_set_metric(vm, "train");
	//show metric scorer parameters
	std::cout << "New training scorer: " << *training_scorer << std::endl;
	qr::metric::ir::Metric* testing_scorer = check_and_set_metric(vm, "test");
	//show metric scorer parameters
	std::cout << "New testing scorer: " << *testing_scorer << std::endl;

	// FILE STUFF
	unsigned int npartialsave = check_and_set<unsigned int>  (vm, "partial", "Partial file save frequency was not set.");
	if (npartialsave > 0)
		r->set_partialsave(npartialsave);

	// TODO: check what can be null, everywhere!!!!
	std::string training_filename = check_and_set<std::string>  (vm, "train", "Training filename was not set.");
	std::string validation_filename = check_and_set<std::string>  (vm, "valid", "Validatin filename was not set.");
	std::string test_filename = check_and_set<std::string>  (vm, "test", "Test filename was not set.");
	std::string features_filename = check_and_set<std::string>  (vm, "features", "Features filename was not set.");
	std::string output_basename = check_and_set<std::string>  (vm, "output", "Output filename was not set.");
	printf("Filenames:\n");
	printf("\ttraining file = %s\n",   training_filename.c_str());
	printf("\tvalidation file = %s\n", validation_filename.c_str());
	printf("\ttest file = %s\n", 	   test_filename.c_str());
	printf("\tfeatures file = %s\n",   features_filename.c_str());
	printf("\toutput basename = %s\n", output_basename.c_str());

	//set seed for rand()
	srand(time(NULL));

	//instantiate a new evaluator with read arguments
	qr::metric::evaluator ev(r, training_scorer, testing_scorer);

	//start evaluation process
	ev.evaluate(training_filename, validation_filename, test_filename, features_filename, output_basename);

	/*
	if (output_basename)
		ev.write();
	*/

	return 0;
}
