# gpu-synth
by Devin Mooers

This is an OpenCL-powered additive synthesizer plugin. It's meant to run on a beefy GPU.

The OpenCL synthesis code is inside `opencl_kernels.cl`. The OpenCL handler code is in `OpenCL.cpp` - this writes/reads OpenCL buffers to/from the kernel/GPU. `VoiceManager.cpp` handles voice management and setting the energy and damping parameters of each voice (which then get fed > OpenCL.cpp > opencl_kernels.cl for synthesis).

gpu-synth uses the WDL-OL plugin framework by Oli Larkin.

#### Warning: this is probably completely broken. Just FYI! This is not plug-and-play. Feel free to gut the code, though, and use it for your own purposes.

License: MIT. Credit appreciated!