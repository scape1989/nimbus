#include "shared/nimbus.h"
#include "projection_app.h"
#define float T

using nimbus::Data;
using nimbus::Job;
using nimbus::Application;

ProjectionApp::ProjectionApp() {
}
;

void ProjectionApp::Load() {
	printf("Worker beginning to load application\n");

	/* Declare and initialize data, jobs and policies. */

	RegisterJob("main", new Main(this));

	printf("Finished creating job and data definitions\n");
	printf("Finished loading application\n");
}

Main::Main(Application *app) {
	set_application(app);
}
;

Job* Main::Clone() {
	printf("Cloning main job\n");
	return new Main(application());
}
;

void Main::Execute(std::string params, const DataArray& da) {
	printf("Begin main\n");
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	std::string par;

	GetNewJobID(&j, 2);
	before.clear();
	after.clear();
	read.clear();
	write.clear();
	after.insert(j[1]);
	SpawnComputeJob("init", j[0], read, write, before, after, par);
	printf("Spawned init\n");

	before.clear();
	after.clear();
	read.clear();
	write.clear();
	before.insert(j[0]);
	SpawnComputeJob("init", j[0], read, write, before, after, par);

	printf("Completed main\n");
}
;

Project_Forloop_Condition::Project_Forloop_Condition(Application* app) {
	set_application(app);
}
;

Job * Project_Forloop_Condition::Clone() {
	std::cout << "Cloning Project_Forloop_Part1 job!\n";
	return new Project_Forloop_Part1(application());
}
;

void Project_Forloop_Condition::Execute(std::string params, const DataArray& da) {
	std::cout << "Executing the Project_Forloop_Part1 job\n";
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	IDSet<partition_t> neighbor_partitions;
	partition_t pid0 = 0, pid1 = 1, pid2 = 2, pid3 = 3, pid4 = 4;
	std::string par;

	char_separator<char> separator("-");
	tokenizer<char_separator<char> > tokens(params, separator);
	tokenizer<char_separator<char> >::iterator iter = tokens.begin();
	int iteration, desired_iterations;
	float global_tolerance;
	nimbus::ParseID(*iter, iteration);
	iter++;
	nimbus::ParseID(*iter, desired_iterations);
	iter++;
	nimbus::ParseID(*iter, global_tolerance);
	
	// input data
	T* residual = (T*) da[0];
	SPARSE_MATRIX_FLAT_NXN<T>* A_pid1 = (SPARSE_MATRIX_FLAT_NXN<T>*) da[1];
	SPARSE_MATRIX_FLAT_NXN<T>* A_pid2 = (SPARSE_MATRIX_FLAT_NXN<T>*) da[2];
	VECTOR_ND<T>* b_interior_pid1 = (VECTOR_ND<T>*) da[3];
	VECTOR_ND<T>* b_interior_pid2 = (VECTOR_ND<T>*) da[4];
	VECTOR_ND<T>* x_interior_pid1 = (VECTOR_ND<T>*) da[5];
	VECTOR_ND<T>* x_interior_pid2 = (VECTOR_ND<T>*) da[6];

	// new data
	GetNewDataID(&d, 16);
	DefineData("temp_interior_pid1", d[0], pid1, neighbor_partitions, par);
	DefineData("temp_interior_pid2", d[1], pid2, neighbor_partitions, par);
	DefineData("local_dot_prod_z_b_pid1", d[2], pid1, neighbor_partitions, par);
	DefineData("local_dot_prod_z_b_pid2", d[3], pid2, neighbor_partitions, par);
	DefineData("rho", d[4], pid0, neighbor_partitions, par);
	DefineData("p_interior_pid1", d[5], pid1, neighbor_partitions, par);
	DefineData("p_interior_pid2", d[6], pid2, neighbor_partitions, par);
	DefineData("p_boundary_pid1", d[7], pid1, neighbor_partitions, par);
	DefineData("p_boundary_pid2", d[8], pid2, neighbor_partitions, par);
	DefineData("local_dot_prod_p_temp_pid1", d[9], pid1, neighbor_partitions, par);
	DefineData("local_dot_prod_p_temp_pid2", d[10], pid2, neighbor_partitions, par);
	DefineData("global_sum", d[11], pid0, neighbor_partitions, par);
	DefineData("rho_old_pid1", d[12], pid1, neighbor_partitions, par);
	DefineData("rho_old_pid2", d[13], pid2, neighbor_partitions, par);
	DefineData("z_interior_pid1", d[14], pid1, neighbor_partitions, par);
	DefineData("z_interior_pid1", d[15], pid2, neighbor_partitions, par);
	
	if(iteration == 1 || iteration < desired_iterations && residual> global_tolerance) {
		// Project_Forloop_Part1, pid = 1
		read.clear();
		read.insert(da[1]); // A_pid1
		read.insert(da[3]); // b_interior_pid1
		write.clear();
		write.insert(d[15]); //z_interior_pid1
		write.insert(d[2]); // local_dot_prod_zb_pid1
		before.clear();
		after.clear(); after.insert(j[2]);
		SpawnComputeJob("Project_Forloop_Part1", j[0], read, write, before, after, par);

		// Project_Forloop_Part1, pid = 2
		read.clear();
		read.insert(da[2]); // A_pid2
		read.insert(da[4]); // b_interior_pid2
		write.clear();
		write.insert(d[16]); //z_interior_pid2
		write.insert(d[3]); // local_dot_prod_zb_pid2
		before.clear();
		after.clear(); after.insert(j[2]);
		SpawnComputeJob("Project_Forloop_Part1", j[1], read, write, before, after, par);

		// Global_Sum
		read.clear();
		read.insert(d[2]); // local_dot_prod_zb_pid1
		read.insert(d[3]); // local_dot_prod_zb_pid2
		write.clear();
		write.insert(d[4]); // rho
		before.clear();
		before.insert(j[0]); // Project_Forloop_Part1, pid = 1
		before.insert(j[1]); // Project_Forloop_Part1, pid = 2
		after.clear();
		after.insert(j[3]); // Project_Forloop_Part2, pid = 1
		after.insert(j[4]); // Project_Forloop_Part2, pid = 2
		SpawnComputeJob("Global_Sum", j[2], read, write, before, after, par);

		// Project_Forloop_Part2, pid = 1
		read.clear();
		read.insert(d[4]); // rho
		read.insert(d[12]); // rho_old_pid1
		read.insert(d[15]); // z_interior_pid1
		read.insert(d[5]); // p_interior_pid1
		write.clear();
		write.insert(d[5]); // p_interior_pid1
		before.clear();
		before.insert(j[2]); // Global_Sum
		after.clear();
		after.insert(j[5]); // Project_Forloop_Part3
		SpawnComputeJob("Project_Forloop_Part2", j[3], read, write, before, after, params);

		// Project_Forloop_Part2, pid = 2
		read.clear();
		read.insert(d[4]); // rho
		read.insert(d[13]); // rho_old_pid2
		read.insert(d[16]); // z_interior_pid2
		read.insert(d[6]); // p_interior_pid2
		write.clear();
		write.insert(d[6]); // p_interior_pid2
		before.clear();
		before.insert(j[2]); // Global_Sum
		after.clear();
		after.insert(j[6]); // Project_Forloop_Part3
		SpawnComputeJob("Project_Forloop_Part2", j[4], read, write, before, after, params);

		// Project_Forloop_Part3, pid = 1
		read.clear();
		read.insert(da[1]); // A_pid1
		read.insert(d[5]); // p_interior_pid1
		read.insert(d[6]); // p_interior_pid2
		write.clear();
		write.insert(d[0]); // tmp_interior_pid1
		write.insert(d[9]); // local_dot_prod_p_temp_pid1
		before.clear();
		before.insert(j[3]);
		before.insert(j[4]);
		after.clear();
		after.insert(j[7]);
		SpawnComputeJob("Project_Forloop_Part3", j[5], read, write, before, after, par);

		// Project_Forloop_Part3, pid = 2
		read.clear();
		read.insert(da[2]); // A_pid2
		read.insert(d[6]); // p_interior_pid2
		read.insert(d[5]); // p_interior_pid1
		write.clear();
		write.insert(d[1]); // tmp_interior_pid2
		write.insert(d[10]); // local_dot_prod_p_temp_pid2
		before.clear();
		before.insert(j[3]);
		before.insert(j[4]);
		after.clear();
		after.insert(j[7]);
		SpawnComputeJob("Project_Forloop_Part3", j[6], read, write, before, after, par);

		// Global_Sum
		read.clear();
		read.insert(d[9]); // local_dot_prod_p_temp_pid1
		read.insert(d[10]); // local_dot_prod_p_temp_pid2
		read.insert(d[11]); // global_sum
		write.clear();
		write.insert(d[11]); // global_sum
		before.clear();
		before.insert(j[5]); // Project_Forloop_Part3, pid = 1
		before.insert(j[6]); // Project_Forloop_Part3, pid = 2
		after.clear();
		after.insert(j[8]); // Project_Forloop_Part4, pid = 1
		after.insert(j[9]); // Project_Forloop_Part4, pid = 2
		SpawnComputeJob("Global_Sum", j[7], read, write, before, after, par);

		// Project_Forloop_Part4, pid = 1
		read.clear();
		read.insert(da[5]); // x_interior_pid1
		read.insert(d[5]); // p_interior_pid1
		read.insert(da[3]); // b_interior_pid1
		read.insert(d[0]); // tmp_interior_pid1
		write.clear();
		write.insert(da[5]); // x_interior_pid1
		write.insert(da[3]); // b_interior_pid1
		before.clear();
		before.insert(j[7]);
		after.clear();
		after.clear(j[10]);
		SpawnComputeJob("Project_Forloop_Part4", j[8], read, write, before, after, par);

		// Project_Forloop_Part4, pid = 2
		read.clear();
		read.insert(da[6]); // x_interior_pid2
		read.insert(d[6]); // p_interior_pid2
		read.insert(da[4]); // b_interior_pid2
		read.insert(d[1]); // tmp_interior_pid2
		write.clear();
		write.insert(da[6]); // x_interior_pid2
		write.insert(da[4]); // b_interior_pid2
		before.clear();
		before.insert(j[7]);
		after.clear();
		after.clear(j[10]);
		SpawnComputeJob("Project_Forloop_Part4", j[9], read, write, before, after, par);

		// Global_Max
		read.clear();
		read.insert(da[3]); // b_interior_pid1
		read.insert(da[4]); // b_interior_pid2
		write.clear();
		write.insert(da[0]); // residual
		before.clear();
		before.insert(j[8]);
		before.insert(j[9]);
		after.clear();
		after.insert(j[11]);
		SpawnComputeJob("Global_Max", j[10], read, write, before, after, par);

		// Project_Forloop_Condition
		read.clear();
		read.insert(da[0]);
		read.insert(da[1]);
		read.insert(da[2]);
		read.insert(da[3]);
		read.insert(da[4]);
		read.insert(da[5]);
		read.insert(da[6]);
		write.clear();
		read.insert(da[1]);
		read.insert(da[2]);
		read.insert(da[3]);
		read.insert(da[4]);
		read.insert(da[5]);
		read.insert(da[6]);
		before.clear();
		before.insert(j[10]);
		SpawnComputeJob("Project_Forloop_Condition", j[11], read, write, before, after, par);
	}
	else {

	}
	// TODO: Fill_Ghost_Cells(x);
};

Project_Forloop_Part1::Project_Forloop_Part1(Application* app) {
	set_application(app);
};

Job * Project_Forloop_Part1::Clone() {
	std::cout << "Cloning Project_Forloop_Part1 job!\n";
	return new Project_Forloop_Part1(application());
};

void Project_Forloop_Part1::Execute(std::string params, const DataArray& da) {
	std::cout << "Executing the Project_Forloop_Part1 job\n";
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	std::string par;


	AC->Solve_Forward_Substitution(b_interior, temp_interior, true); // diagonal should be treated as the identity
	AC->Solve_Backward_Substitution(temp_interior, z_interior, false, true); // diagonal is inverted to save on divides
};

Project_Forloop_Part2::Project_Forloop_Part2(Application* app) {
	set_application(app);
};

Job * Project_Forloop_Part2::Clone() {
	std::cout << "Cloning Project_Forloop_Part2 job!\n";
	return new Project_Forloop_Part2(application());
};

void Project_Forloop_Part2::Execute(std::string params, const DataArray& da) {
	std::cout << "Executing the Project_Forloop_Part2 job\n";
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	std::string par;

};

Project_Forloop_Part3::Project_Forloop_Part3(Application* app) {
	set_application(app);
};

Job * Project_Forloop_Part3::Clone() {
	std::cout << "Cloning Project_Forloop_Part3 job!\n";
	return new Project_Forloop_Part3(application());
};

void Project_Forloop_Part3::Execute(std::string params, const DataArray& da) {
	std::cout << "Executing the Project_Forloop_Part3 job\n";
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	std::string par;

};

Project_Forloop_Part4::Project_Forloop_Part4(Application* app) {
	set_application(app);
};

Job * Project_Forloop_Part4::Clone() {
	std::cout << "Cloning Project_Forloop_Part4 job!\n";
	return new Project_Forloop_Part4(application());
};

void Project_Forloop_Part4::Execute(std::string params, const DataArray& da) {
	std::cout << "Executing the Project_Forloop_Part4 job\n";
	std::vector<job_id_t> j;
	std::vector<data_id_t> d;
	IDSet<data_id_t> read, write;
	IDSet<job_id_t> before, after;
	std::string par;
};

template<class T_GRID> void PCG_SPARSE_MPI<T_GRID>::Initialize_Datatypes() {
	MPI_UTILITIES::Free_Elements_And_Clean_Memory(boundary_datatypes);
	MPI_UTILITIES::Free_Elements_And_Clean_Memory(ghost_datatypes);
	boundary_datatypes.Resize(partition.number_of_sides);
	ghost_datatypes.Resize(partition.number_of_sides);
	for (int s=1; s<=partition.number_of_sides; s++)
		if (partition.neighbor_ranks(s)!=MPI::PROC_NULL) {
			if (partition.boundary_indices(s).m) {
				const ARRAY<int>& displacements=partition.boundary_indices(s);
				ARRAY<int> block_lengths(displacements.m, false);
				ARRAYS_COMPUTATIONS::Fill(block_lengths, 1);
				boundary_datatypes(s)=MPI_UTILITIES::Datatype<T>().Create_indexed(displacements.m,&block_lengths(1),&displacements(1)); // TODO: collapse consecutive elements into blocks
				boundary_datatypes(s).Commit();
			}
			int ghost_indices_length=partition.ghost_indices(s).Size()+1;
			if (ghost_indices_length) {
				ghost_datatypes(s)=MPI_UTILITIES::Datatype<T>().Create_indexed(1,&ghost_indices_length,&partition.ghost_indices(s).min_corner);
				ghost_datatypes(s).Commit();
			}
		}
}
