#include "helpers.h"
// I suggest testing with this set to 1 first! 88888
#define MAX_THREADS 8

/*
simulate population change for one community of one species
update its population based on the input `local_population_share` 
 Note: 1) The change in population for a community is proportional to 
          its growth_rate and local_population_share
       2) If it has collected enough food to feed the population it grows, else it shrinks
       3) If the population drops below 5 it goes extinct
*/
void update_community_population(Species_Community *communities, int community_id, float local_population_share) {
  //std::atomic<int> population;         // the population of a speciies
  //float growth_rate;                  // growth_rate for this species community
  //bool flag;                          //extinct

  float growth_rate = communities[community_id].growth_rate;
  int food_collected = communities[community_id].food_collected;

  if(food_collected >= (communities[community_id].population + growth_rate*local_population_share)){
    //grow
    communities[community_id].population = communities[community_id].population + growth_rate*local_population_share;
  }else{
    communities[community_id].population = communities[community_id].population - growth_rate*local_population_share;
    //shrink
  }
  if(communities[community_id].population<5){
    communities[community_id].population = 0;
  }
}

/* 
find the local population share for one community of one species
Population share:
  the percentage of  population in a region that is a given species 
  across all communities of all species
*/ 
float compute_local_population_share(Species_Community *communities, int community_id){
  float local_population_share;
  //for every communinty who has the same region_of_world
  //1. calculate the overall population in the region same region_of_world
  //2. calculate the overall population aross communities that have the same region_of_world and species_type
  //3. calculate percentage

  int given_region = communities[community_id].region_of_world;  
  int given_species = communities[community_id].species_type; 

  std::atomic<int> given_species_population = {0}; 
  std::atomic<int> all_species_population = {0}; //same region

  for (int id = 0; id < TOTAL_COMMUNITIES; id++){
    int region = communities[id].region_of_world;
    int species = communities[id].species_type;   
    int community_population = communities[id].population;

    if(region == given_region){
      all_species_population += community_population;
      if(species == given_species){
        given_species_population += community_population;
      }
    }
  }
  local_population_share =  given_species_population/ all_species_population;
  return local_population_share;
}
void update_all_populations_helper(Species_Community *communities, int i, int max_size_per_thread){
  int start_id = i*max_size_per_thread;
  int end_id = start_id+max_size_per_thread-1;

  while(start_id<=end_id){
    float local_population_share = compute_local_population_share(communities,start_id);
    update_community_population(communities,start_id,local_population_share);
    start_id++;
  }
}

/*
updates the population for all communities of all species
launch MAX_THREADS 
thread safe > Warning there likely is a data dependancy!
helper functions if that makes your life easier!
*/
void update_all_populations(Species_Community *communities){
  std::thread threads[MAX_THREADS];
  int max_size_per_thread = (TOTAL_COMMUNITIES+MAX_THREADS-1)/MAX_THREADS;

  for(int i = 0;i<MAX_THREADS;i++){
    threads[i] = std::thread(&update_all_populations_helper,communities,i,max_size_per_thread);
  }
  for (int i=0; i<MAX_THREADS; i++) {
    threads[i].join();
  }

}
void food_oracle_helper(int community_id,int max_size_per_thread,int i,int*partial){
  int start = i*max_size_per_thread;
  int end = start+max_size_per_thread-1;

  while(start<=end){
    int prev = *partial;
    *partial = prev+food_oracle(community_id);
    start++;
  }
}

/*
simulate food gathering
Each round food starts at 0 
  Each member of the population tries to collect food
food_oracle(): get a new amount of food for EACH member of the population
MAX_THREADS threads per Species_Community: Not spread across them but for each one)
*/
void gather_all_food(Species_Community *communities) {
  // food_oracle(int community_id)
  // MAX_THREADS threads of parallelism to 
  // compute the updated food count for each community of each species
  std::thread threads[MAX_THREADS];

  for (int community_id = 0; community_id < TOTAL_COMMUNITIES; community_id++){
    int community_sum = 0;
    int member = communities[community_id].population;
    int max_size_per_thread = (member+MAX_THREADS-1)/MAX_THREADS;
    int partial = 0;

    for(int i = 0;i<MAX_THREADS;i++){
      threads[i] = std::thread(&food_oracle_helper,community_id,max_size_per_thread,i,&partial);
    }

    for(int i = 0;i<MAX_THREADS;i++){
      threads[i].join();
    }
    
    communities[community_id].food_collected = community_sum;
  }
}



// Hgathers food and updates the population for everyone
//        for NUM_TIME_PERIODS
void population_dynamics(Species_Community *communities){
  // for NUM_TIME_PERIODS
  // for all population, food_oracle(int community_id)
  for(int i = 0;i< NUM_TIME_PERIODS ;i++){
    gather_all_food(communities);
    update_all_populations(communities);
  }
}