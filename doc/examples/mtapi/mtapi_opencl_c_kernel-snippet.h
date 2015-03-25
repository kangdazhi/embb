const char * kernel =
"__kernel void test(\n"
"  __global void* arguments,\n"
"  int arguments_size,\n"
"  __global void* result_buffer,\n"
"  int result_buffer_size,\n"
"  __global void* node_local_data,\n"
"  int node_local_data_size) {\n"
"  int ii = get_global_id(0);\n"
"  int elements = arguments_size / sizeof(float) / 2;\n"
"  if (ii >= elements)"
"    return;"
"  __global float* a = (__global float*)arguments;\n"
"  __global float* b = ((__global float*)arguments) + elements;\n"
"  __global float* c = (__global float*)result_buffer;\n"
"  __global float* d = (__global float*)node_local_data;\n"
"  c[ii] = a[ii] + b[ii] + d[0];\n"
"}\n";