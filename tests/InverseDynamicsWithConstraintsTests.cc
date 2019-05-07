#include <UnitTest++.h>
#include "rbdl/rbdl.h"
#include <cassert>

#include <chrono>
#include <thread>
#include <iostream>

using namespace std;
using namespace RigidBodyDynamics;
using namespace RigidBodyDynamics::Math;

const double TEST_PREC = 1.0e-11;

const bool flag_printTimingData = false;

void calcCuboidInertia(double mass,
                       double xLength,
                       double yLength,
                       double zLength,
                       Vector3d& iner){
    iner[0] = (mass/12.)*(yLength*yLength + zLength*zLength);
    iner[1] = (mass/12.)*(xLength*xLength + zLength*zLength);
    iner[2] = (mass/12.)*(yLength*yLength + xLength*xLength);
}

struct PlanarBipedFloatingBase {
  PlanarBipedFloatingBase()
    : model()
      ,cs()
      ,q()
      ,qd()
      ,qdd()
      ,tau()
      ,pelvisWidth(0.2)
      ,segmentLength(0.5)
      ,pelvisMass(50.0)
      ,segmentMass(1.0)
      ,X_p(Xtrans(Vector3d(0.,0.,-segmentLength)))
      ,X_s(Xtrans(Vector3d(0.,0.,0.))) {

    Body segment = Body(segmentMass,
                        Vector3d(0, 0., -0.5 * segmentLength),
                        Vector3d(0.,segmentMass*segmentLength*segmentLength/3.,0.));
    Body pelvis  = Body(pelvisMass, Vector3d(0, 0., 0.),
                        Vector3d(0.,pelvisMass*pelvisWidth*pelvisWidth/3.,0.));

    Joint joint_TxzRy = Joint(SpatialVector(0.,0.,0., 1.,0.,0.),
                              SpatialVector(0.,0.,0., 0.,0.,1.),
                              SpatialVector(0.,1.,0., 0.,0.,0.));

    Joint joint_Ry = Joint(SpatialVector(0.,1.,0., 0.,0.,0.));

    idxPelvis       = model.AddBody(0,
                                    Xtrans(Vector3d(0.,0.,0.)),
                                    joint_TxzRy, pelvis);
    idxLeftUpperLeg  = model.AddBody(idxPelvis,
                                    Xtrans(Vector3d(-pelvisWidth*0.5,0.,0.)),
                                    joint_Ry,segment);
    idxLeftLowerLeg = model.AddBody(idxLeftUpperLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Ry, segment);
    idxRightUpperLeg  = model.AddBody(idxPelvis,
                                    Xtrans(Vector3d( pelvisWidth*0.5,0.,0.)),
                                    joint_Ry,segment);
    idxRightLowerLeg = model.AddBody(idxRightUpperLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Ry, segment);

    cs.AddContactConstraint(idxLeftLowerLeg, Vector3d(0.,0.,-segmentLength),
                                             Vector3d(1.,0.,0.));
    cs.AddContactConstraint(idxLeftLowerLeg, Vector3d(0.,0.,-segmentLength),
                                             Vector3d(0.,0.,1.));
    cs.AddContactConstraint(idxRightLowerLeg, Vector3d(0.,0.,-segmentLength),
                                              Vector3d(1.,0.,0.));
    cs.AddContactConstraint(idxRightLowerLeg, Vector3d(0.,0.,-segmentLength),
                                              Vector3d(0.,0.,1.));

    cs.Bind(model);
    q     = VectorNd::Zero(model.dof_count);
    qd    = VectorNd::Zero(model.dof_count);
    qdd   = VectorNd::Zero(model.dof_count);
    tau   = VectorNd::Zero(model.dof_count);

  }

  Model model;
  ConstraintSet cs;

  VectorNd q;
  VectorNd qd;
  VectorNd qdd;
  VectorNd tau;

  double pelvisWidth;
  double segmentLength;
  double pelvisMass;
  double segmentMass;

  unsigned int idxPelvis ;
  unsigned int idxLeftUpperLeg  ;
  unsigned int idxLeftLowerLeg  ;
  unsigned int idxRightUpperLeg ;
  unsigned int idxRightLowerLeg ;

  SpatialTransform X_p;
  SpatialTransform X_s;

};


struct SpatialBipedFloatingBase {
  SpatialBipedFloatingBase()
    : model()
      ,cs()
      ,q()
      ,qd()
      ,qdd()
      ,tau()
      ,pelvisWidth(0.2)
      ,segmentLength(0.5)
      ,footLength(0.3)
      ,pelvisMass(50.0)
      ,segmentMass(5.0)
      ,footMass(1.0)
      ,X_p(Xtrans(Vector3d(0.,0.,-segmentLength)))
      ,X_s(Xtrans(Vector3d(0.,0.,0.))) {

    /*
      From the biped's perspective
      x: forward
      z: up
      y: left
    */

    Vector3d segmentInertia, pelvisInertia, footInertia;
    calcCuboidInertia(segmentMass,
                      0.1*segmentLength,
                      0.1*segmentLength,
                      segmentLength,
                      segmentInertia);
    calcCuboidInertia(pelvisMass,
                      0.75*pelvisWidth,
                           pelvisWidth,
                      0.50*pelvisWidth,
                      pelvisInertia);
    calcCuboidInertia(footMass,
                          footLength,
                      0.1*footLength,
                      0.1*footLength,
                      footInertia);

    Body segment = Body(segmentMass,
                        Vector3d(0, 0., -0.5 * segmentLength),
                        segmentInertia);
    Body pelvis  = Body(pelvisMass,
                        Vector3d(0, 0., 0.),
                        pelvisInertia);

    Body foot    = Body(footMass,
                        Vector3d(0.5*footLength,0.,0.),
                        footInertia);

    Joint joint_TxyzRxyz = Joint(SpatialVector(0.,0.,0., 1.,0.,0.),
                                 SpatialVector(0.,0.,0., 0.,1.,0.),
                                 SpatialVector(0.,0.,0., 0.,0.,1.),
                                 SpatialVector(1.,0.,0., 0.,0.,0.),
                                 SpatialVector(0.,1.,0., 0.,0.,0.),
                                 SpatialVector(0.,0.,1., 0.,0.,0.));

    Joint joint_Ry  = Joint(SpatialVector(0.,1.,0., 0.,0.,0.));

    Joint joint_Rxy = Joint(SpatialVector(1.,0.,0., 0.,0.,0.),
                            SpatialVector(0.,1.,0., 0.,0.,0.));
    Joint joint_Rxyz = Joint(SpatialVector(1.,0.,0., 0.,0.,0.),
                             SpatialVector(0.,1.,0., 0.,0.,0.),
                             SpatialVector(0.,0.,1., 0.,0.,0.));


    idxPelvis       = model.AddBody(0,
                                    Xtrans(Vector3d(0.,0.,0.)),
                                    joint_TxyzRxyz, pelvis);

    idxLeftUpperLeg  = model.AddBody(idxPelvis,
                                    Xtrans(Vector3d(-pelvisWidth*0.5,0.,0.)),
                                    joint_Rxyz,segment);
    idxLeftLowerLeg = model.AddBody(idxLeftUpperLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Ry, segment);
    idxLeftFoot     = model.AddBody(idxLeftLowerLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Rxy,
                                    foot);

    idxRightUpperLeg  = model.AddBody(idxPelvis,
                                    Xtrans(Vector3d( pelvisWidth*0.5,0.,0.)),
                                    joint_Rxyz,segment);
    idxRightLowerLeg = model.AddBody(idxRightUpperLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Ry, segment);
    idxRightFoot     = model.AddBody(idxRightLowerLeg,
                                    Xtrans(Vector3d(0.,0.,-segmentLength)),
                                    joint_Rxy,
                                    foot);

    SpatialTransform X_zero = Xtrans(Vector3d(0.,0.,0.));
    bool baumgarteEnabled = false;
    double timeStabilityInverse = 0.1;

    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,1,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,0,1,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,0,0,1),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(1,0,0,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,1,0,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxLeftFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,1,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);

    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,1,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,0,1,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,0,0,0,1),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(1,0,0,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,1,0,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);
    cs.AddLoopConstraint(idxRightFoot, 0,X_zero,X_zero,
                                      SpatialVector(0,0,1,0,0,0),
                                      baumgarteEnabled,timeStabilityInverse);

    cs.Bind(model);
    q     = VectorNd::Zero(model.dof_count);
    qd    = VectorNd::Zero(model.dof_count);
    qdd   = VectorNd::Zero(model.dof_count);
    tau   = VectorNd::Zero(model.dof_count);

  }

  Model model;
  ConstraintSet cs;

  VectorNd q;
  VectorNd qd;
  VectorNd qdd;
  VectorNd tau;

  double pelvisWidth;
  double segmentLength;
  double footLength;
  double pelvisMass;
  double segmentMass;
  double footMass;

  unsigned int idxPelvis ;
  unsigned int idxLeftUpperLeg  ;
  unsigned int idxLeftLowerLeg  ;
  unsigned int idxLeftFoot;
  unsigned int idxRightUpperLeg ;
  unsigned int idxRightLowerLeg ;
  unsigned int idxRightFoot;

  SpatialTransform X_p;
  SpatialTransform X_s;

};


TEST_FIXTURE(PlanarBipedFloatingBase, TestCorrectness) {

  //1. Make the simple biped
  //2. Assemble it to a specific q and qdot.

  VectorNd q0, qd0, weights, qddTarget, lambda, qddFwd,qddErr,
           lambdaIdc, lambdaFwd, lambdaErr;
  MatrixNd K;
  std::vector<bool> dofActuated;

  unsigned int n  = unsigned (int (q.rows()));
  //unsigned int na = unsigned (int(q.rows()))-3;
  //unsigned int nc = unsigned (int(cs.name.size()));

  q0.resize(n);
  qd0.resize(n);
  weights.resize(n);
  qddTarget.resize(n);
  qddFwd.resize(n);
  qddErr.resize(n);
  dofActuated.resize(n);



  for(unsigned int i=0; i<n;++i){
    if(i>=3){
      dofActuated[i] = true;
    }else{
      dofActuated[i] = false;
    }
  }

  //All of the ugly numbers below are pseudo random numbers generated
  //from Matlab to ensure that the method works, and that the tests are
  //not passing due to some quirk of symmetry in the state of the model.
  q0[0] = 0;
  q0[1] = 1.0;
  q0[2] = 0;
  q0[3] = 5.472155299638031e-01;
  q0[4] = 1.386244428286791e-01;
  q0[5] = 1.492940055590575e-01;
  q0[6] = 2.575082541237365e-01;

  qd0[0] = 1.965952504312082e-01;
  qd0[1] = 2.510838579760311e-01;
  qd0[2] = 6.160446761466392e-01;
  qd0[3] = 4.732888489027293e-01;
  qd0[4] = 3.516595070629968e-01;
  qd0[5] = 8.308286278962909e-01;
  qd0[6] = 5.852640911527243e-01;

  for(unsigned int i =0; i< q.rows();++i){
    weights[i]    = 1;
  }

  qddTarget[0]  = 5.497236082911395e-01;
  qddTarget[1]  = 9.171936638298100e-01;
  qddTarget[2]  = 2.858390188203735e-01;
  qddTarget[3]  = 7.572002291107213e-01;
  qddTarget[4]  = 7.537290942784953e-01;
  qddTarget[5]  = 3.804458469753567e-01;
  qddTarget[6]  = 5.678216407252211e-01;


  CalcAssemblyQ(model,q0,cs,q,weights);
  CalcAssemblyQDot(model,q,qd0,cs,qd,weights);
  

  //6. Call InverseDynamicsConstraints & repeat setps 4 & 5
  VectorNd tauIDC = VectorNd::Zero(tau.size());
  VectorNd qddIDC = VectorNd::Zero(qdd.size());

  cs.SetActuationMap(model, dofActuated);
  InverseDynamicsConstraints(model,
                             q,qd,qddTarget,
                             cs, qddIDC,tauIDC);
  lambdaIdc = cs.force;
  ForwardDynamicsConstraintsDirect(model,q,qd,tauIDC,cs,qddFwd);
  lambdaFwd = cs.force;


  qddErr = qddIDC-qddFwd;
  lambdaErr=lambdaIdc-lambdaFwd;
  for(unsigned int i=0; i<q.rows();++i){
    CHECK_CLOSE(qddIDC[i], qddFwd[i], TEST_PREC);
  }
  for(unsigned int i=0; i<lambdaFwd.rows();++i){
    CHECK_CLOSE(lambdaIdc[i], lambdaFwd[i], TEST_PREC);
  }

  //Timing comparision    
  if(flag_printTimingData){
    unsigned int iterations = 100;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(unsigned int i=0; i<iterations; ++i){
      InverseDynamicsConstraints(model,
                                 q,qd,qddTarget,
                                 cs,qddIDC,tauIDC);
    }
    auto t2   = std::chrono::high_resolution_clock::now();
    auto tidc = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);

    t1 = std::chrono::high_resolution_clock::now();
    for(unsigned int i=0; i<iterations; ++i){
      ForwardDynamicsConstraintsDirect(model,q,qd,tauIDC,cs,qddFwd);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto tfd = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);

    std::cout << "Planar Biped Dof: " << model.dof_count << std::endl;
    std::cout << "Cost per evaluation : us, xfd " << std::endl;
    std::cout << "IDCns: " << double(tidc.count()) / double(iterations) << " "
                        << double(tidc.count())/double(tfd.count()) << std::endl;
    std::cout << "FD:    " << double(tfd.count()) / double(iterations) << " "
                        << double(tfd.count())/double(tfd.count()) << std::endl;
  }

}


TEST_FIXTURE(SpatialBipedFloatingBase, TestCorrectness) {

  //1. Make the simple spatial biped
  //2. Assemble it to a specific q and qdot.

  VectorNd q0, qd0, weights, qddTarget, lambda, qddFwd,qddErr,
           lambdaFwd,lambdaIdc,lambdaErr;
  MatrixNd K;
  std::vector<bool> dofActuated;

  unsigned int n  = unsigned( int( q.rows()));
  //unsigned int na = unsigned( int( q.rows()))-6;
  unsigned int nc = unsigned( int( cs.name.size()));

  q0.resize(n);
  qd0.resize(n);
  weights.resize(n);
  qddTarget.resize(n);
  qddFwd.resize(n);
  qddErr.resize(n);
  lambda.resize(nc);
  dofActuated.resize(n);

  for(unsigned int i=0; i<n;++i){
    if(i>=6){
      dofActuated[i] = true;
    }else{
      dofActuated[i] = false;
    }
  }

  //All of the ugly numbers below are pseudo random numbers generated
  //from Matlab to ensure that the method works, and that the tests are
  //not passing due to some quirk of symmetry in the state of the model.

  q0[0] = 7.922073295595544e-01;
  q0[1] = 9.594924263929030e-01;
  q0[2] = 6.557406991565868e-01;
  q0[3] = 3.571167857418955e-02;
  q0[4] = 8.491293058687771e-01;
  q0[5] = 9.339932477575505e-01;
  q0[6] = 6.787351548577735e-01;
  q0[7] = 7.577401305783334e-01;
  q0[8] = 7.431324681249162e-01;
  q0[9] = 3.922270195341682e-01;
  q0[10] = 6.554778901775566e-01;
  q0[11] = 1.711866878115618e-01;
  q0[12] = 7.060460880196088e-01;
  q0[13] = 3.183284637742068e-02;
  q0[14] = 2.769229849608900e-01;
  q0[15] = 4.617139063115394e-02;
  q0[16] = 9.713178123584754e-02;
  q0[17] = 8.234578283272926e-01;


  qd0[0]  = 6.948286229758170e-01;
  qd0[1]  = 3.170994800608605e-01;
  qd0[2]  = 9.502220488383549e-01;
  qd0[3]  = 3.444608050290876e-02;
  qd0[4]  = 4.387443596563982e-01;
  qd0[5]  = 3.815584570930084e-01;
  qd0[6]  = 7.655167881490024e-01;
  qd0[7]  = 7.951999011370632e-01;
  qd0[8]  = 1.868726045543786e-01;
  qd0[9]  = 4.897643957882311e-01;
  qd0[10] = 4.455862007108995e-01;
  qd0[11] = 6.463130101112646e-01;
  qd0[12] = 7.093648308580726e-01;
  qd0[13] = 7.546866819823609e-01;
  qd0[14] = 2.760250769985784e-01;
  qd0[15] = 6.797026768536748e-01;
  qd0[16] = 6.550980039738407e-01;
  qd0[17] = 1.626117351946306e-01;

  for(unsigned int i =0; i< q.rows();++i){
    weights[i]    = 1.;
  }

  qddTarget[0]  = 1.189976815583766e-01;
  qddTarget[1]  = 4.983640519821430e-01;
  qddTarget[2]  = 9.597439585160811e-01;
  qddTarget[3]  = 3.403857266661332e-01;
  qddTarget[4]  = 5.852677509797773e-01;
  qddTarget[5]  = 2.238119394911370e-01;
  qddTarget[6]  = 7.512670593056529e-01;
  qddTarget[7]  = 2.550951154592691e-01;
  qddTarget[8]  = 5.059570516651424e-01;
  qddTarget[9]  = 6.990767226566860e-01;
  qddTarget[10] = 8.909032525357985e-01;
  qddTarget[11] = 9.592914252054443e-01;
  qddTarget[12] = 5.472155299638031e-01;
  qddTarget[13] = 1.386244428286791e-01;
  qddTarget[14] = 1.492940055590575e-01;
  qddTarget[15] = 2.575082541237365e-01;
  qddTarget[16] = 8.407172559836625e-01;
  qddTarget[17] = 2.542821789715310e-01;


  CalcAssemblyQ(model,q0,cs,q,weights);
  CalcAssemblyQDot(model,q,qd0,cs,qd,weights);


  //6. Call InverseDynamicsConstraints

  VectorNd tauIDC = VectorNd::Zero(tau.size());
  VectorNd qddIDC = VectorNd::Zero(qdd.size());
  cs.SetActuationMap(model,dofActuated);
  InverseDynamicsConstraints(model,
                             q,qd,qddTarget,
                             cs, qddIDC,tauIDC);
  lambdaIdc = cs.force;

  ForwardDynamicsConstraintsDirect(model,q,qd,tauIDC,cs,qddFwd);
  lambdaFwd = cs.force;

  qddErr = qddIDC-qddFwd;
  lambdaErr= lambdaIdc-lambdaFwd;
  for(unsigned int i=0; i<q.rows();++i){
    CHECK_CLOSE(qddIDC[i], qddFwd[i], TEST_PREC);
  }
  for(unsigned int i=0; i<lambdaIdc.rows();++i){
    CHECK_CLOSE(lambdaIdc[i], lambdaFwd[i], TEST_PREC);
  }

  //Timing comparision
  if(flag_printTimingData){
    unsigned int iterations = 100;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(unsigned int i=0; i<iterations; ++i){
      InverseDynamicsConstraints(model,
                                 q,qd,qddTarget,
                                 cs, qddIDC,tauIDC);
    }
    auto t2     = std::chrono::high_resolution_clock::now();
    auto tidc = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);

    t1 = std::chrono::high_resolution_clock::now();
    for(unsigned int i=0; i<iterations; ++i){
      ForwardDynamicsConstraintsDirect(model,q,qd,tauIDC,cs,qddFwd);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto tfd = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);

    std::cout << "Spatial Biped Dof: " << model.dof_count << std::endl;
    std::cout << "Cost per evaluation : us, xfd" << std::endl;
    std::cout << "IDCns: " << double(tidc.count())/double(iterations) << " "
                        << double(tidc.count())/double(tfd.count()) << std::endl;
    std::cout << "FD:    " << double(tfd.count())/double(iterations) << " "
                        << double(tfd.count())/double(tfd.count()) << std::endl;
  }
}
