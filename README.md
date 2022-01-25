# Sensor Daemon

Reads data from physical ports and writes data to shmem for other processes to use.

## Dependencies
 - Clang
 - libi2c-dev
 - libgpiod-dev

## Creating a new sensor module

This deamon spawns variable many sensors with variably many interfaces. For example, imagine we have 2 ultrasound 
sensors and a hall effect sensor. We can instantiate of the following, and each will run in their own thread. 

It is, therefor, necessary that each sensor module follows the following restricitions: 

 1) The module must be stateless. That is, all memory managment (references to the sensor obj, etc) are not handled by
the module. Any call to any function must always return the same result. 

 2) The module must implement the following function with the following interfaces
   
	a) An init function in the form `void *init(void **) {...}`. The return must be the object reference that is used
to interface with the remainder of the interface
   
	b) A read function in the form `double read(void *sensor_object) {...}`. The return is the value read and must be
a double. 
   
	c) A cleanup function int he form `void cleanup(void *)` which cleans up the object and all memory associated with
it.

## Interfacing sensor module with program

To add a sensor to this object, be sure to have a module that abides by the restrictions outlined above.
Adding a sensor requires a single call to the sensor module. Instantiation, threading, etc is handled by the module.
To add a sensor, call the `int add_sensor(void \*init, void \*\*init_args, void \*cleanup, void \*read, int interval)`
where `init` is the function to instantiate sensor instance, `init_args` is a malloc´d ptr to the parameters needed by
the init function, `cleanup`is the pointer to the cleanup function, `read` is the pointer to the read function, and 
interval is the number of useconds to wait between calling read. 

## Design choices

This interface was created to allow scalable sensor polling. The `usleep(interval)`was inspired by the long latency 
gpiod\_events suffer (and hence poor timing results) because of their intterupt nature. Calling sleep allows the kernel
to handle the distribution of events better than before, without interfering in any data-critical sections of the 
sensors. 
