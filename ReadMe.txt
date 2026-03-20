-------------------------------------------------------------------

--------------FramWork Code for Geant4

     Version: 1.0      Date:2021.09
     Geant4 Version: Geant4.10.07   
-------------------------------------------------------------------

This code constructs a simple geometric structure, 
with basic model building, data acquisition and other functions. 
For different needs, users can add and modify the code.


HOW TO RUN

    - Execute B1 in the 'interactive mode' with visualization:
        Idle> make clean
        Idle> make 
        Idle> B1
      and type in the commands from run.mac line by line:  
        Idle> /control/verbose 2
        Idle> /tracking/verbose 1
        Idle> /run/beamOn 10 
        Idle> ...
        Idle> exit
      or
        Idle> /control/execute run.mac
        ....
        Idle> exit

    - Execute exampleB1  in the 'batch' mode from macro files 
      (without visualization)
        Idle> B1 run.mac

-------------------------------------------------------------------
--------------CopyRight 
-------------------------------------------------------------------
Bilibili/知乎 : 美猴王子
Research interest : 
      Nuclear physics,Neutron Nuclear Reaction,Photonuclear Physics
Email : lixinx@mail.ustc.edu.cn
Institute/Address:
Shanghai Laser Electron Gamma Source (SLEGS)
Shanghai Synchrotron Radiation Facility (SSRF)
239 Zhangheng Road, Pudong New Area, Shanghai, China. 201204
 
