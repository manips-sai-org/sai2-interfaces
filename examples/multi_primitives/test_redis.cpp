/*
 * Example of a controller for a Kuka arm made with the motion arm primitive for redundant arms
 *
 */

#include <iostream>
#include <string>
#include <thread>
#include <math.h>
#include <Eigen/Dense>
#include "redis/RedisClient.h"

using namespace std;
RedisClient redis_client;

const string KEY = "test_key";

int main (int argc, char** argv) {
	// start redis client
	HiredisServerInfo info;
	info.hostname_ = "127.0.0.1";
	info.port_ = 6379;
	info.timeout_ = { 1, 500000 }; // 1.5 seconds
	redis_client = RedisClient();
	redis_client.serverIs(info);

	// set value
	Eigen::VectorXd vector_set = Eigen::VectorXd::Zero(5);
	vector_set << 1.1, 2.0, 3.0, 4.0, 5.0;
	redis_client.setEigenVectorDerivedExpanded(KEY, vector_set);

	// get value
	Eigen::VectorXd vector_get = Eigen::VectorXd::Zero(5);
	redis_client.getEigenVectorDerivedExpanded(KEY, vector_get);
	cout << vector_get << endl;

	return 0;
}
