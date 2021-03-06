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

#define EPS 0.00001

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of
	//   x, y, theta and their uncertainties from GPS) and all weights to 1.
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	if (is_initialized){
		return;
	}

	is_initialized = true;

	num_particles = 100;

	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);

	for (int i = 0; i < num_particles; i++){
		Particle particle;
		particle.id = i;
		particle.x = dist_x(generator);
		particle.y = dist_y(generator);
		particle.theta = dist_theta(generator);
		particle.weight = 1;


		weights.push_back(1.0);
		particles.push_back(particle);
	}

}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/


	normal_distribution<double> dist_x(0, std_pos[0]);
	normal_distribution<double> dist_y(0, std_pos[1]);
	normal_distribution<double> dist_theta(0, std_pos[2]);

	for (int i = 0; i < num_particles; i++){
  	double theta = particles[i].theta;

    if ( fabs(yaw_rate) < EPS ) {
			cout << "Small yaw_rate" << '\n';
      particles[i].x += velocity * delta_t * cos( theta );
      particles[i].y += velocity * delta_t * sin( theta );
    } else {
      particles[i].x += velocity / yaw_rate * ( sin( theta + yaw_rate * delta_t ) - sin( theta ) );
      particles[i].y += velocity / yaw_rate * ( cos( theta ) - cos( theta + yaw_rate * delta_t ) );
      particles[i].theta += yaw_rate * delta_t;
    }

    particles[i].x += dist_x(generator);
    particles[i].y += dist_y(generator);
    particles[i].theta += dist_theta(generator);
  }

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
	//   implement this method and use it as a helper during the updateWeights phase.

	for (int i = 0; i < observations.size(); i++){
		double closest_dis = numeric_limits<double>::max();
		int closest_idx = -1;

		for (int j = 0; j < predicted.size(); j++){
			double x_dis = predicted[j].x - observations[i].x;
			double y_dis = predicted[j].y - observations[i].y;

			if (x_dis * x_dis + y_dis * y_dis < closest_dis){
				closest_dis = x_dis * x_dis + y_dis * y_dis;
				closest_idx = j;
			}
			observations[i].id = closest_idx;
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

	vector<LandmarkObs> landmarks;
	for (int i = 0; i < map_landmarks.landmark_list.size(); i++){
		landmarks.push_back(LandmarkObs{map_landmarks.landmark_list[i].id_i,double(map_landmarks.landmark_list[i].x_f), double(map_landmarks.landmark_list[i].y_f)});
	}


	for (int i = 0; i < num_particles; i++){
		vector<LandmarkObs> transformedObservations;
		for(unsigned int j = 0; j < observations.size(); j++) {
			double xx = cos(particles[i].theta)*observations[j].x - sin(particles[i].theta)*observations[j].y + particles[i].x;
			double yy = sin(particles[i].theta)*observations[j].x + cos(particles[i].theta)*observations[j].y + particles[i].y;
			transformedObservations.push_back(LandmarkObs{ observations[j].id, xx, yy });
		}

		dataAssociation(landmarks, transformedObservations);

		for (int j = 0; j < transformedObservations.size(); j++){
			double dX = transformedObservations[j].x - landmarks[transformedObservations[j].id].x;
			double dY = transformedObservations[j].y - landmarks[transformedObservations[j].id].y;
			double weight = ( 1/(2*M_PI*std_landmark[0]*std_landmark[1])) * exp( -( dX*dX/(2*std_landmark[0]*std_landmark[0]) + (dY*dY/(2*std_landmark[1]*std_landmark[1])) ) );
			if (weight == 0){
				particles[i].weight *= EPS;
				cout << "Zero wights!" << '\n';
			}
			else{
				particles[i].weight *= weight;
			}
		}
		weights[i] = particles[i].weight;
	}

	double total_weight = 0;
	for (int i = 0; i < num_particles; i++){
		total_weight += weights[i];
	}
	for (int i = 0; i < num_particles; i++){
		weights[i] /= total_weight;
		particles[i].weight /= total_weight;
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight.
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	double max_weight = *(max_element(weights.begin(), weights.end()));

	uniform_real_distribution<double> distDouble(0.0, max_weight);
	uniform_int_distribution<int> distInt(0, num_particles - 1);

	int index = distInt(generator);

  double beta = 0.0;

	vector<Particle> resampledParticles;
  for(int i = 0; i < num_particles; i++) {
    beta += distDouble(generator) * 2.0;
    while( beta > weights[index]) {
      beta -= weights[index];
      index = (index + 1) % num_particles;
    }
    resampledParticles.push_back(particles[index]);
  }
  particles = resampledParticles;
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations,
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
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
