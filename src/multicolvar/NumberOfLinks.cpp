/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2013-2016 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "MultiColvarFunction.h"
#include "core/ActionRegister.h"
#include "tools/SwitchingFunction.h"

#include <string>
#include <cmath> 

//+PLUMEDOC MCOLVARF NLINKS 
/*
Calculate number of pairs of atoms/molecules that are "linked"

In its simplest guise this coordinate calculates a coordination number.  Each pair
of atoms is assumed "linked" if they are within some cutoff of each other.  In more 
complex applications each entity is a vector and this quantity measures whether
pairs of vectors are (a) within a certain cutoff and (b) if the two vectors have 
similar orientations.  The vectors on individual atoms could be Steinhardt parameters
(see \ref Q3, \ref Q4 and \ref Q6) or they could describe some internal vector in a molecule.

\par Examples

The following calculates how many bonds there are in a system containing 64 atoms and outputs
this quantity to a file.

\verbatim
DENSITY SPECIES=1-64 LABEL=d1
NLINKS ARG=d1 SWITCH={RATIONAL D_0=1.3 R_0=0.2} LABEL=dd
PRINT ARG=dd FILE=colvar
\endverbatim

The following calculates how many pairs of neighbouring atoms in a system containg 64 atoms have
similar dispositions for the atoms in their coordination sphere.  This calculation uses the 
dot product of the Q6 vectors on adjacent atoms to measure whether or not two atoms have the same
``orientation"

\verbatim
Q6 SPECIES=1-64 SWITCH={RATIONAL D_0=1.3 R_0=0.2} LABEL=q6
NLINKS ARG=q6 SWITCH={RATIONAL D_0=1.3 R_0=0.2} LABEL=dd
PRINT ARG=dd FILE=colvar
\endverbatim 

*/
//+ENDPLUMEDOC

namespace PLMD {
namespace multicolvar {

class NumberOfLinks : public MultiColvarFunction {
private:
/// The values of the quantities in the dot products
  std::vector<double> orient0, orient1; 
/// The switching function that tells us if atoms are close enough together
  SwitchingFunction switchingFunction;
public:
  static void registerKeywords( Keywords& keys );
  explicit NumberOfLinks(const ActionOptions&);
/// Do the stuff with the switching functions
  void calculateWeight( AtomValuePack& myatoms ) const ;
/// Actually do the calculation
  double compute( const unsigned& tindex, AtomValuePack& myatoms ) const ;
/// Is the variable periodic
  bool isPeriodic(){ return false; }
};

PLUMED_REGISTER_ACTION(NumberOfLinks,"NLINKS")

void NumberOfLinks::registerKeywords( Keywords& keys ){
  MultiColvarFunction::registerKeywords( keys );
  keys.add("compulsory","NN","6","The n parameter of the switching function ");
  keys.add("compulsory","MM","12","The m parameter of the switching function ");
  keys.add("compulsory","D_0","0.0","The d_0 parameter of the switching function");
  keys.add("compulsory","R_0","The r_0 parameter of the switching function");
  keys.add("optional","SWITCH","This keyword is used if you want to employ an alternative to the continuous swiching function defined above. "
                               "The following provides information on the \\ref switchingfunction that are available. "
                               "When this keyword is present you no longer need the NN, MM, D_0 and R_0 keywords.");
  // Use actionWithDistributionKeywords
  keys.remove("LOWMEM"); 
  keys.addFlag("LOWMEM",false,"lower the memory requirements");
}

NumberOfLinks::NumberOfLinks(const ActionOptions& ao):
Action(ao),
MultiColvarFunction(ao)
{
  // The weight of this does have derivatives
  weightHasDerivatives=true;

  // Read in the switching function
  std::string sw, errors; parse("SWITCH",sw);
  if(sw.length()>0){
     switchingFunction.set(sw,errors);
  } else {
     double r_0=-1.0, d_0; int nn, mm;
     parse("NN",nn); parse("MM",mm);
     parse("R_0",r_0); parse("D_0",d_0);
     if( r_0<0.0 ) error("you must set a value for R_0");
     switchingFunction.set(nn,mm,r_0,d_0);
  }
  log.printf("  calculating number of links with atoms separation of %s\n",( switchingFunction.description() ).c_str() );
  buildAtomListWithPairs( false ); setLinkCellCutoff( switchingFunction.get_dmax() ); 

  for(unsigned i=0;i<getNumberOfBaseMultiColvars();++i){
    if( !getBaseMultiColvar(i)->hasDifferentiableOrientation() ) error("cannot use multicolvar of type " + getBaseMultiColvar(i)->getName() );
  }

  // Create holders for the collective variable
  readVesselKeywords();
  plumed_assert( getNumberOfVessels()==0 );
  std::string input; addVessel( "SUM", input, -1 );
  readVesselKeywords();
}

void NumberOfLinks::calculateWeight( AtomValuePack& myatoms ) const {
  Vector distance = getSeparation( myatoms.getPosition(0), myatoms.getPosition(1) );
  double dfunc, sw = switchingFunction.calculateSqr( distance.modulo2(), dfunc );
  myatoms.setValue(0,sw);

  if( !doNotCalculateDerivatives() ){
      CatomPack atom0=getCentralAtomPackFromInput( myatoms.getIndex(0) );
      myatoms.addComDerivatives( 0, (-dfunc)*distance, atom0 );
      CatomPack atom1=getCentralAtomPackFromInput( myatoms.getIndex(1) );
      myatoms.addComDerivatives( 0, (dfunc)*distance, atom1 );
      myatoms.addBoxDerivatives( 0, (-dfunc)*Tensor(distance,distance) ); 
  }
}

double NumberOfLinks::compute( const unsigned& tindex, AtomValuePack& myatoms ) const {
   if( getBaseMultiColvar(0)->getNumberOfQuantities()<3 ) return 1.0; 

   unsigned ncomp=getBaseMultiColvar(0)->getNumberOfQuantities();

   std::vector<double> orient0( ncomp ), orient1( ncomp );
   getVectorForTask( myatoms.getIndex(0), true, orient0 ); 
   getVectorForTask( myatoms.getIndex(1), true, orient1 );

   double dot=0;
   for(unsigned k=2;k<orient0.size();++k){
       dot+=orient0[k]*orient1[k];
   }

   if( !doNotCalculateDerivatives() ){
     unsigned nder=myatoms.getNumberOfDerivatives();   //getNumberOfDerivatives();
     MultiValue myder0(ncomp,nder), myder1(ncomp,nder);
     getVectorDerivatives( myatoms.getIndex(0), true, myder0 );
     mergeVectorDerivatives( 1, 2, orient1.size(), myatoms.getIndex(0), orient1, myder0, myatoms );
     getVectorDerivatives( myatoms.getIndex(1), true, myder1 ); 
     mergeVectorDerivatives( 1, 2, orient0.size(), myatoms.getIndex(1), orient0, myder1, myatoms );
   }

   return dot;
}

}
}
