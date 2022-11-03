# Task-Aware HIP Library

## Documentation and programming model
The Task-Aware HIP (TAHIP) library extends the functionality of the HIP language by providing new mechanisms for improving the interoperability between parallel task-based programming models, such as OpenMP and OmpSs-2, and the offloading of kernels to AMD GPUs. The library allows the efficient offloading of HIP operations from concurrent tasks in an asynchronous manner. TAHIP manages most of the low-level aspects, so the developers can focus in exploiting the parallelism os the application.

The TAHIP library contains both exclusive TAHIP functions and wrapper functions of the HIP language and its APIs. All the wrapper functions refer to the most relevant HIP asynchronous operations, so if more information about the original functions is needed it can be found in the [HIP Toolkit Documentation](https://rocmdocs.amd.com/en/latest/Programming_Guides/HIP-GUIDE.html).

The idea behind the library is to let the tasks inside a taskified application to offload multiple HIP operations into a given set of HIP streams that run on the AMD GPU, and then these tasks should bind their completion to the finalization of their operations in an asynchronous way. All of this happens without blocking the host CPUs while the HIP operations are running on the GPU, so this allows host CPUs to execute other ready tasks meanwhile. In general, tasks will use standard HIP functions to offload operations and use TAHIP functions to bind and "synchronize" with their compeltion.

The library is initialized by calling tahipInit, which is a wrapper function of the cuInit function of the Driver API. The TAHIP library is finalized by calling tahipFinalize. In the TAHIP applications is important to use multiple stream, as multiple tasks will submit operations on HIP streams concurrently and we want to distribute the operations and reduce unnecessary dependencies (which are implicitly created). For this reason, the TAHIP library generates a pool of streams that can be used by the tasks during the execution of the application. This pool can be created with the function ```tahipCreateStreams(size_t count)```, and must be destroyed before the finalization of the library with ```tahipDestroyStreams```. 
When a task wants to offload an operation to the GPU, it can retrieve on of these streams for exclusive use until it is returned. When the stream is not needed anymore, the task can return it to the pool. The functions corresponding to these actions are the following:

```
tahipGetStream(hipStream_t *stream);
tahipReturnStream(hipStream_t stream);
```

When operations have been offloaded from a task, it may be interesting for the task to synchronize with these operations. That's when the function ```tahipSynchronizeStreamAsync(hipStream_t stream)``` comes to use. Given the asynchronous nature of HIP operations, this function allows the task to synchronize with the operations submitted to a specific stream, but in an asynchronous manner. It binds the completion of the task to the finalization of the submitted operations on the stream, and so the calling task will be able to continue and finish its execution but will delay its fully completion (and the release of data dependencies) until all the operations in the stream finalize.

TAHIP also defines another way to asynchronously wait for HIP operations that allows a more fine-grained waiting. As it was mentioned earlier, TAHIP defines a wrapper function for each of the most relevant HIP asynchronous operations. The wrappers have the same behaviour and parameters as the originals, plus an additional parameter that returns a TAHIP request (tahipRequest). If the request pointer parameter is not NULL, TAHIP generates a tahipRequest and saves a pointer to it in the output parameter after executing the corresponding operation. Then, the task can bind itself to the request calling the functions ```tahipWaitRequestAsync(tahipRequest *request)``` or ```tahipWaitallRequestsAsync(size_t count, tahipRequest requests[])```. If the request pointer is NULL, TAHIP directly binds the desired operation to the calling task without generating any request. As an example, the following codes do exactly the same:

**1:**

```
tahipRequest request;
tahipMemcpyAsync(..., stream, &request);
tahipWaitRequestAsync(&request);
```

**2:**

```
tahipMemcpyAsync(..., stream, NULL);
```

Finally, inside the header file src/include/TAHIP.h, it can be found a brief explanation of every function in the library and their functionality (the remaining functions of the library are all wrapper functions), which can be used as a "cheat sheet" and it is very helpful while using the TAHIP library. 

## Building and installing
TAHIP uses the standard GNU automake and libtool toolchain. When cloning from a repository, the building environment must be prepared executing the bootstrap script:

```
$ ./bootstrap
```

Then, to configure, compile and install the library execute the following commands:

```
$ ./configure --prefix=$INSTALL_PREFIX --with-hip=$HIP_HOME --with-hipblas=$HIPBLAS_HOME ..other options..
$ make
$ make install
```

where $INSTALL_PREFIX is the directory into which to install TAHIP, and both $HIP_HOME and $HIPBLAS_HOME are the prefixes of the HIP and hipBLAS installations. There are two other optional configuration flags, which probably will not be needed if the user has a normal HIP setup:

- `--with-hip-bin` : It lets the user specify the directory where the HIP binary files are installed. It's important to remark that hipconfig is expected to be found here, since it will be used to get the proper configuration flags.
- `--with-hip-lib` : It lets the user specify the directory where the HIP libraries are installed.

- `--with-hipblas-include` : It lets the user specify the directory where the hipBLAS binary files are installed.
- `--with-hip-lib` : It lets the user specify the directory where the hipBLAS libraries are installed.

Once TAHIP is built and installed, e.g, in $TAHIP_HOME, the installation folder will contain the library in $TAHIP_HOME/lib (or $TAHIP_HOME/lib64) and the header in $TAHIP_HOME/include.

## Requirements
In order to install the TAHIP library, the main requirements are the following:

- Automake, autoconf, libtool, make and a C and C++ compiler.
- HIP
- hipBLAS
- Boost library 1.59 or greater
- OmpSs-2 (version 2018.11 or greater)
- ...

The hipBLAS API is not essential to the library, so if the user does the proper changes to the Makefile.am file (basically removing or commenting all the lines containing `src/c/hipBLASOperations.cpp` and a few compiler flags) it can be compiled and installed without it.
