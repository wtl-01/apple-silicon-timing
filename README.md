**IMPORTANT: PLEASE READ THE SRD REPO WIKI BEFORE INSTALLING ANYTHING ONTO THE SRD, AS WELL AS THIS DOCUMENT**

## Motivation
This project originally started with an intention to see if the rowhammer attack was reproducible on apple silicon based devices, as part of my CSC494 project at the University of Toronto. While I faced many obstacles along the way involving converting what is usually an attack mostly researched on x86/linux to work on ARM-Based Apple Silicon, those challenges informed a lot about the unique and secure architecture that apple silicon is based on, and further helped me understand the complexities of the logics of the rowhammer attack.

## Project Scope
I first started by trying my hand on a DDR3 based system. Then, I got started working on translating the code that worked on the DDR3 system to adapt the rowhammer attack to apple silicon. Apple silicon presents unique challenges. First and foremost, I was able to ascertain that the analogue to clflush, known as DC CIVAC [4], which flushes the address from the processor's caches so as to actually access the memory in RAM rather than in the cache, is nonfunctional on Apple Silicon. I also determined that on Apple, the most accurate format of timing involved spawning a separate timing thread and reading between counters, like used in Branch Different [2], with traditional POSIX and rdtsc analogues unavailable or too coarse in granularity. MacOS does not provide a similar framework to the pagemap interface on Linux which reduces our ability to find addresses in the same bank. All in all, the lack of effective instructions in user space for cache eviction, the lack of an accurate in-OS timer, and lack of methods to determine physical to virtual memory address mappings present additional difficulties in adapting the rowhammer attack onto Apple Silicon.

## Instructions
To Use this Repository, You must first have access to the Apple SRD Repository (apple/security-research-device). For more information about this program, see https://developer.apple.com/programs/security-research-device/. It is recommended that you read thoroughly through the repo's wiki to gain an idea how programs can be run on the SRD.

Basically, the SRD is able to flash software onto itself by means of what is called a "cryptex" by Apple. If you put executables that are configured like this one into the cryptex source, the example-cryptex will pick up this project, alongside others, then build it using XCode SDKs to cross-compile it into a Mach-O executable (able to be run on Apple Silicon arm64e), then flash it into a cryptex on the SRD iPhone, where it will hold the cross compiled and compatible binaries. You can then ssh into the device's root user, and run the executable after locating it.

Once you have access to the security-research-device repo, simply clone it: ie, `git clone https://github.com/apple/security-research-device.git`

Once cloned, follow the following exact steps to run code from this repository. These steps are for the histogram executable:
1. Go into the source folder for the example cryptex project: `cd example-crpytex/src`.
2. Begin following the SRD example-cryptex setup instructions: https://github.com/apple/security-research-device/tree/main/example-cryptex. Note that you must be in possession of the Security Research Device and have it be connected to an Apple Silicon Mac machine updated to the newest MacOS version the SRD programme supports. At the time of writing, it is MacOS 14.4 Sonoma. Using an Intel Mac will currently result in errors. See the SRD repo page issue #125 for details.
3. Run make install and verify that the example-cryptex can successfully install onto the iPhone. On the SRD Repo, they recommend you first get the IP of your SRD, then "run `nc ${IP} 7777`, and if you see 'Hello!' it's all working!.
4. Add this repository as a submodule dependency in that directory: `git submodule add https://github.com/wliu416/rowhammer-apple`. Note: This is incredibly important: this repo is set up exactly to be configured as such and to be built and installed on the SRD within the cryptex installation process.
5. Run make install in the example-cryptex directory. If the settings are correct, the cross compilation on Mac OS should work.
6. If make install is successful, you should be able to see the compiled executables: notably histogram on the cryptex.
7. SSH onto the SRD by using the provided ssh server on the cryptex that was installed. Then run which {executable} to find where the executable you want to run is located.
8. cd into the repo containing your executable.
9. Run the executable. Note redirection and other shell functionality is janky. An easy way is to just collect any data or executable output from stdout, where you should output measurements.

**Note:** Read the Apple Documentation Carefully. If you do some things like accept Over the Air (OTA) updates, **you could permanently brick the SRD**. Find the documentation here: https://github.com/apple/security-research-device/wiki

## Findings
The SRD is a slightly finicky platform to figure out for a newbie, but once you navigate around the barriers, it is a rewarding platform. So far I have been able to experiment with timing parameters, and establish the most effective form of timing: that of measuring clock cycles through a separate timing thread. I also determined that cache invalidation and clear instructions like `DC CIVAC` are not useful when run in the userspace, which requires potential attacks like rowhammer to pursue advanced, fast cache-eviction methods like (Vila et al, 2019) [5], or to somehow multitask and evict addresses by simply attacking another address. This is in stark contrast to other side channel and Rowhammer attack papers that also use ARM processors, like the Cortex A-53, where `DC CIVAC` and analogues to `clflush` have demonstrated excellent usability [1]. 

MacOS also does not have a super convenient virtual to physical memory address like the Linux `/proc/pid/pagemap`, which would require the reconstruction of virtual memory to DIMM bank mappings in the worst case, should one be not easily available. At the end of the day, the breadth of security research on Apple Silicon pales to that on the more established platforms like x86, but it is certainly an important area to study in an age where ARM-based systems are increasingly being adopted by users in a variety of applications.

## What's Next?
Potential next steps include the implementation of fast cashe eviction methods to replace `DC CIVAC` and `clflush` used in traditional cache based memory attacks like Rowhammer, or by somehow evicting from the cache by simply attacking other columns, with the latter perhaps being able to reach faster speeds. One potential pitfall is if the cache eviction method is too slow, then there will not likely be a requisite amount of row activations before each refresh to actually cause bit flips. Regardless, this points to a potential security benefit in architectures that do not implement cache flush operations in the user space. Traditional timing measurements like timespec are also extremely coarse, granularly speaking on Apple Silicon [2]. This may reduce the feasibility of cache-based attacks that depend on timing side channels to establish the effectiveness of a potential attack. Potentially, a GPU side attack like that previously demonstrated on Android devices could also be feasible, and also very dangerous on Apple silicon based devices [3].

## Links
[1] https://github.com/0x5ec1ab/rowhammer_armv8  
[2] https://www.researchgate.net/publication/361531820_Branch_Different_-_Spectre_Attacks_on_Apple_Silicon  
[3] https://www.vusec.net/wp-content/uploads/2018/05/glitch.pdf  
[4] https://dl.acm.org/doi/pdf/10.1145/3266444.3266454  
[5] https://arxiv.org/abs/1810.01497

