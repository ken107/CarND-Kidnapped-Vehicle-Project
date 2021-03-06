/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of
	//   x, y, theta and their uncertainties from GPS) and all weights to 1.
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	num_particles = 10;

	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);

	for (int i=0; i<num_particles; i++) {
		Particle p;
		p.id = i;
		p.x = dist_x(gen);
		p.y = dist_y(gen);
		p.theta = dist_theta(gen);
		p.weight = 1;
		particles.push_back(p);
	}

	is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
	normal_distribution<double> dist_x(0, std_pos[0]);
	normal_distribution<double> dist_y(0, std_pos[1]);
	normal_distribution<double> dist_theta(0, std_pos[2]);

	for (auto& p : particles) {
		if (yaw_rate == 0) {
			p.x += velocity * delta_t * cos(p.theta);
			p.y += velocity * delta_t * sin(p.theta);
		}
		else {
			p.x += (velocity / yaw_rate) * (sin(p.theta + yaw_rate * delta_t) - sin(p.theta)) + dist_x(gen);
			p.y += (velocity / yaw_rate) * (cos(p.theta) - cos(p.theta + yaw_rate * delta_t)) + dist_y(gen);
			p.theta += yaw_rate * delta_t + dist_theta(gen);
		}
	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
	//   implement this method and use it as a helper during the updateWeights phase.
	for (auto& obs : observations) {
		double nearest = 99999;
		for (int i=0; i<predicted.size(); i++) {
			const auto& pred = predicted[i];
			auto dist = calculateDist(obs.x, obs.y, pred.x, pred.y);
			if (dist < nearest) {
				nearest = dist;
				obs.id = i;
			}
		}
	}
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
	for (auto& p : particles) {
		std::vector<LandmarkObs> predicted;
		for (const auto& landmark : map_landmarks.landmark_list) {
			auto dist = calculateDist(p.x, p.y, landmark.x_f, landmark.y_f);
			if (dist < sensor_range) {
				LandmarkObs lobs;
				lobs.id = landmark.id_i;
				lobs.x = landmark.x_f;
				lobs.y = landmark.y_f;
				predicted.push_back(lobs);
			}
		}

		std::vector<LandmarkObs> transformed;
		for (const auto& obs : observations) {
			auto tobs = transformObservation(obs, p);
			transformed.push_back(tobs);
		}

		dataAssociation(predicted, transformed);

		double weight = 1;
		for (const auto& obs : transformed) {
			const auto& landmark = predicted[obs.id];
			weight *= calculateProbability(obs, landmark, std_landmark);
		}
		p.weight = weight;
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight.
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	vector<double> weights;
	for (const auto& p : particles)
		weights.push_back(p.weight);

	discrete_distribution<int> dist(weights.begin(), weights.end());
	vector<int> indices;
	for (int i=0; i<particles.size(); i++)
		indices.push_back(dist(gen));

	vector<Particle> resampled;
	for (auto i : indices)
		resampled.push_back(particles[i]);

	particles = resampled;
}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations= associations;
 	particle.sense_x = sense_x;
 	particle.sense_y = sense_y;

 	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}

double ParticleFilter::calculateDist(double x1, double y1, double x2, double y2) {
	double dx = x2-x1;
	double dy = y2-y1;
	return sqrt(dx*dx + dy*dy);
}

LandmarkObs ParticleFilter::transformObservation(const LandmarkObs& obs, const Particle& p) {
	LandmarkObs result;
	result.id = 0;
	result.x = p.x + (cos(p.theta) * obs.x) - (sin(p.theta) * obs.y);
	result.y = p.y + (sin(p.theta) * obs.x) + (cos(p.theta) * obs.y);
	return result;
}

double ParticleFilter::calculateProbability(const LandmarkObs& obs, const LandmarkObs& landmark, const double std_landmark[]) {
	double denominator = 2 * M_PI * std_landmark[0] * std_landmark[1];
	double exponent1 = (obs.x - landmark.x) * (obs.x - landmark.x) / (2 * std_landmark[0] * std_landmark[0]);
	double exponent2 = (obs.y - landmark.y) * (obs.y - landmark.y) / (2 * std_landmark[1] * std_landmark[1]);
	double result = exp(-exponent1-exponent2) / denominator;
	return result;
}
