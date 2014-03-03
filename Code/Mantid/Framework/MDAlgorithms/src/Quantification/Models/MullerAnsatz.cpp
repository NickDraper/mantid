// Includes
#include "MantidMDAlgorithms/Quantification/Models/MullerAnsatz.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"

#include <cmath>

namespace Mantid
{
  namespace MDAlgorithms
  {
    DECLARE_FOREGROUNDMODEL(MullerAnsatz);

    using Kernel::Math::BoseEinsteinDistribution;

    struct AnsatzParameters
    {
      /*
      p(1)	A, the intensity scale factor in the Mueller Ansatz formalism.
      !		p(2)	J (maximum of lower bound is at pi*J/2)
      !
      !		p(19) = 1,2,3 for chains along a*, b*, c* respectively 
      !				Default is along c* if any other value given
      !
      !		p(20) = 0		isotropic magnetic form factor
      !			  = 1,2,3	"dx2-y2" form factor, normal to plane of orbital being a*, b*, c* respectively
      !				Default is isotropic form factor

      */
      /// Enumerate parameter positions
      enum {Ampliture,J_bound};

      /// Number of parameters
      enum{NPARAMS = 2};
      /// Parameter names, same order as above
      static char * PAR_NAMES[NPARAMS];
      /// N attrs
      enum {NATTS = 3};
      /// Attribute names
      static char * ATTR_NAMES[NATTS];

    };

    char* AnsatzParameters::PAR_NAMES[AnsatzParameters::NPARAMS] = {"Amplitude","J_boundLimit"};
    char* AnsatzParameters::ATTR_NAMES[AnsatzParameters::NATTS] = {"IonName","ChainDirection","MagneticFFDirection"};
    //static 
    /**
    * Initialize the model
    */
    void MullerAnsatz::init()
    {
      // Default form factor. Can be overridden with the FormFactorIon attribute
      //setFormFactorIon("Fe2"); 

      // Declare parameters that participate in fitting
      for(unsigned int i = 0; i < AnsatzParameters::NPARAMS; ++i)
      {
        declareParameter(AnsatzParameters::PAR_NAMES[i], 0.0);
      }

      // Declare fixed attributes
      declareAttribute(AnsatzParameters::ATTR_NAMES[0], API::IFunction::Attribute("Fe"));
      for(unsigned int i = 1; i < AnsatzParameters::NATTS; ++i)
      {
        declareAttribute(AnsatzParameters::ATTR_NAMES[i], API::IFunction::Attribute(int(1)));
      }
    }

    /**
    * Called when an attribute is set from the Fit string
    * @param name :: The name of the attribute
    * @param attr :: The value of the attribute
    */
    void MullerAnsatz::setAttribute(const std::string & name, const API::IFunction::Attribute& attr)
    {

      if(name == AnsatzParameters::ATTR_NAMES[1])
      {
        m_ChainDirection = ChainDirection(attr.asInt());
      }
      else if(name == AnsatzParameters::ATTR_NAMES[2])
      {
        m_FFDirection = MagneticFFDirection(attr.asInt());
      }
      else if(name == AnsatzParameters::ATTR_NAMES[0])
      {
        setFormFactorIon(attr.asString()); 
      }
      else 
        ForegroundModel::setAttribute(name, attr); // pass it on the base
    }

    static const double TWO_PI = 2.*M_PI;
    double inline amff_cu3d(const double &qsqr,const double &cosbsqr)
    {
      /**
      * Calculates the magnetic form factor for a cu2+ ion with hole having 3dx^2-y2 orbital symmetry 
      * Follows Shamoto et al. PRB 48, 13817(1993) 
      */

      double  t11(0.0232), t12(34.969), t13(0.4023), t14(11.564), t15(0.5882),t16(3.843), t17(-0.0137),    
        t21(1.5189), t22(10.478), t23(1.1512), t24(3.813), t25(0.2918), t26(1.398), t27(0.0017),                   
        t31(-0.3914),t32(14.740), t33(0.1275), t34(3.384), t35(0.2548), t36(1.255), t37(0.0103);

      if (qsqr <= 1.e-6)
      {
        return 1.;
      }
      else
      {
        double ssqr=qsqr/(16.0*M_PI*M_PI);
        double j0= t11*exp(-t12*ssqr)+t13*exp(-t14*ssqr)+t15*exp(-t16*ssqr)+t17;
        double j2=(t21*exp(-t22*ssqr)+t23*exp(-t24*ssqr)+t25*exp(-t26*ssqr)+t27)*ssqr;
        double j4=(t31*exp(-t32*ssqr)+t33*exp(-t34*ssqr)+t35*exp(-t36*ssqr)+t37)*ssqr;
        double amff_cu3d_internal=j0 - (5.0/7.0)*(1.0-3.0*cosbsqr)*j2 +
          (9.0/56.0)*(1.0-10.0*cosbsqr+(35.0/3.0)*(cosbsqr*cosbsqr))*j4;

        return amff_cu3d_internal;

      }
    }
      /**
      * Calculates the scattering intensity
      * @param exptSetup :: Details of the current experiment
      * @param point :: The axis values for the current point in Q-W space: Qx, Qy, Qz, DeltaE. These contain the U matrix
      * rotation already.
      * @return The weight contributing from this point
      */
      double MullerAnsatz::scatteringIntensity(const API::ExperimentInfo & exptSetup, const std::vector<double> & point) const
      {
        const double sigma_mag(289.6);	// (gamma*r0)**2  in mbarn

        const double qx(point[0]), qy(point[1]), qz(point[2]), eps(point[3]);
        const double qsqr = qx*qx + qy*qy + qz*qz;
        const double Amplitude = getCurrentParameterValue(AnsatzParameters::Ampliture);
        const double J_bound = getCurrentParameterValue(AnsatzParameters::J_bound);

        //  const double epssqr = eps*eps;


        double weight,qchain;
        double qh,qk,ql,arlu1,arlu2,arlu3;
        ForegroundModel::convertToHKL(exptSetup,qx,qy,qz,qh,qk,ql,arlu1,arlu2,arlu3);

        //	Orientation of the chain:
        switch(m_ChainDirection)
        {
        case(Along_a):
          {
            qchain = qh;
            break;
          }
        case(Along_b):
          {
            qchain = qk;
            break;
          }
        case(Along_c):
          {
            qchain = qk;
            break;
          }
        default:
          qchain = ql;
        }
        double wl = M_PI_2*J_bound*std::fabs(sin(TWO_PI*qchain));
        double wu = M_PI*J_bound*std::fabs(sin(M_PI*qchain));
        if (eps > (wl+FLT_EPSILON) && eps <= wu)
        {
          //	Orientation of the hole orbital
          double formfactor;
          switch(m_FFDirection)
          {
          case(NormalTo_a):
            { 
              double  cos_beta_sqr = (qh*arlu1)*(qh*arlu1)/ qsqr;
              formfactor = amff_cu3d(qsqr,cos_beta_sqr);
              break;
            }
          case(NormalTo_b):
            {
              double cos_beta_sqr = (qk*arlu2)*(qk*arlu2)/ qsqr;
              formfactor = amff_cu3d(qsqr,cos_beta_sqr);
              break;
            }
          case(NormalTo_c):
            {
              double cos_beta_sqr = (ql*arlu3)*(ql*arlu3)/ qsqr;
              formfactor = amff_cu3d(qsqr,cos_beta_sqr);
              break;
            }
          case(Isotropic): 
          default:
            {
              double formfactor = this->formFactor(qsqr);
            }
          }

          const double tempInK = exptSetup.getLogAsSingleValue("temperature_log");
          const double boseFactor = BoseEinsteinDistribution::np1Eps(eps,tempInK);
          weight = Amplitude*(sigma_mag/M_PI)* (boseFactor/eps) * (formfactor*formfactor) /  sqrt((eps-wl)*(eps+wl));
          return weight;
        }
        else
          return 0;

        /*



        iorient = nint(p(20))
        qsqr = qx**2 + qy**2 + qz**2
        if (iorient .eq. 1) then
        cos_beta_sqr = (qh*arlu(1))**2 / qsqr
        elseif (iorient .eq. 2) then
        cos_beta_sqr = (qk*arlu(2))**2 / qsqr
        elseif (iorient .eq. 3) then
        cos_beta_sqr = (ql*arlu(3))**2 / qsqr
        endif

        !	Get spectral weight
        wl = piby2*p(2)*abs(sin(twopi*qchain))
        wu = pi*p(2)*abs(sin(pi*qchain))
        if (eps .gt. (wl+small) .and. eps .le. wu) then
        if (iorient .ge. 1 .and. iorient .le. 3) then
        weight = p(1)*(sigma_mag/pi)* (bose(eps,temp)/eps) *
        &                                   (amff_cu3d(qsqr,cos_beta_sqr))**2 / sqrt(eps**2-wl**2)
        else
        weight = p(1)*(sigma_mag/pi)* (bose(eps,temp)/eps) * (form_table(qsqr))**2 / sqrt(eps**2-wl**2)
        endif  
        else
        weight = 0.0d0
        endif
        */



        return weight;
      }
    }
  }
