#include"Perturbations.h"

//====================================================
// Constructors
//====================================================

Perturbations::Perturbations(
    BackgroundCosmology *cosmo, 
    RecombinationHistory *rec) : 
  cosmo(cosmo), 
  rec(rec)
{}

//====================================================
// Do all the solving
//====================================================

void Perturbations::solve(){

  // Integrate all the perturbation equation and spline the result
  integrate_perturbations();

  // Compute source functions and spline the result
  
  //compute_source_functions();
}

//====================================================
// The main work: integrate all the perturbations
// and spline the results
//====================================================

void Perturbations::integrate_perturbations(){
  Utils::StartTiming("integrateperturbation");
  //===================================================================
  // TODO: Set up the k-array for the k's we are going to integrate over
  // Start at k_min end at k_max with n_k points with either a
  // quadratic or a logarithmic spacing
  //===================================================================
  Vector k_array(n_k);
  double dk = (log10(Constants.k_max) - log10(Constants.k_min)) / (n_k - 1.0);
  double c = Constants.c;
  double Hp;
  double H0 = cosmo -> get_H0();
  double OmegaR0 = cosmo -> get_OmegaR(0);
  double dtaudx;
  double len_tc;
  int x_full_index;
  Vector x_all = Utils::linspace(x_start, x_end, n_x);
  Vector x_tc;
  Vector x_full;
  Vector Psi (n_x * n_k);
  Vector Phi (n_x * n_k);
  Vector delta_cdm (n_x * n_k);
  Vector delta_b (n_x * n_k);
  Vector v_cdm (n_x * n_k);
  Vector v_b (n_x * n_k);
  Vector Theta0 (n_x * n_k);
  Vector Theta1 (n_x * n_k);
  Vector Theta2 (n_x * n_k);
  Vector Theta3 (n_x * n_k);
  Vector Theta4 (n_x * n_k);
  Vector Theta5 (n_x * n_k);
  Vector Theta6 (n_x * n_k);
  Vector Theta7 (n_x * n_k);
  std::vector<Vector> Thetas = {Theta0, Theta1, Theta2, Theta3,
                                Theta4, Theta5, Theta6, Theta7};
  // Loop over all wavenumbers

  for(int ik = 0; ik < n_k; ik++){
    k_array[ik] = log10(Constants.k_min) + ik * dk;
    k_array[ik] = pow(10, k_array[ik]);
    // Progress bar...
    if( (10*ik) / n_k != (10*ik+10) / n_k ) {
      std::cout << (100*ik+100)/n_k << "% " << std::flush;
      if(ik == n_k-1) std::cout << std::endl;
    }

    // Current value of k
    double k = k_array[ik];

    // Find value to integrate to
    double x_end_tight = get_tight_coupling_time(k);

    for (int ix = 0; ix < n_x; ix++){
      if (x_all[ix] >= x_end_tight){
        len_tc = ix;
        x_tc = Utils::linspace(x_start, x_all[ix - 1], ix);
        x_full = Utils::linspace(x_all[ix], x_end, n_x - ix);
        break;
      }
    }

    //===================================================================
    // TODO: Tight coupling integration
    // Remember to implement the routines:
    // set_ic : The IC at the start
    // rhs_tight_coupling_ode : The dydx for our coupled ODE system
    //===================================================================

    // Set up initial conditions in the tight coupling regime
    auto y_tight_coupling_ini = set_ic(x_start, k);

    // The tight coupling ODE system
    ODEFunction dydx_tight_coupling = [&](double x, const double *y, double *dydx){      
      return rhs_tight_coupling_ode(x, k, y, dydx);
    };

    // Integrate from x_start -> x_end_tight
    // ...
    // ...
    // ...
    // ...
    // ...
    // Setting up ODE for tightly coupled regime

    ODESolver ode_tc;

    // Solving ODE and extracting solution array from ODEsolver
    ode_tc.solve(dydx_tight_coupling, x_tc, y_tight_coupling_ini);
    auto all_data_tc = ode_tc.get_data();

    for (int jx = 0; jx < len_tc; jx++){
      Hp                        = cosmo -> Hp_of_x(x_tc[jx]);
      dtaudx                    = rec   -> dtaudx_of_x(x_tc[jx]);
      
      Phi[jx + n_x * ik]        = all_data_tc[jx][Constants.ind_Phi_tc];
      delta_cdm[jx + n_x * ik]  = all_data_tc[jx][Constants.ind_deltacdm_tc];
      delta_b[jx + n_x * ik]    = all_data_tc[jx][Constants.ind_deltab_tc];
      v_cdm[jx + n_x * ik]      = all_data_tc[jx][Constants.ind_vcdm_tc];
      v_b[jx + n_x * ik]        = all_data_tc[jx][Constants.ind_vb_tc];
      Thetas[0][jx + n_x * ik]  = all_data_tc[jx][Constants.ind_start_theta_tc];
      Thetas[1][jx + n_x * ik]  = all_data_tc[jx][Constants.ind_start_theta_tc + 1];
      Thetas[2][jx + n_x * ik]  = - 20 * c * k / (45 * Hp * dtaudx) * Thetas[1][jx + n_x * ik];
        
      for (int ell = 3; ell < Constants.n_ell_theta; ell++){
          Thetas[ell][jx + n_x * ik] = - ell / (2.0 * ell + 1.0) * c * k / (Hp * dtaudx) * Thetas[ell - 1][jx + n_x * ik];
        }

      Psi[jx + n_x * ik] = - Phi[jx + n_x * ik] 
                           - 12.0 * H0 * H0 / (c * c * k * k * exp(2 * x_tc[jx]))
                           * OmegaR0 * Thetas[2][jx + n_x * ik];
    }

    //====i===============================================================
    // TODO: Full equation integration
    // Remember to implement the routines:
    // set_ic_after_tight_coupling : The IC after tight coupling ends
    // rhs_full_ode : The dydx for our coupled ODE system
    //===================================================================

    // Set up initial conditions (y_tight_coupling is the solution at the end of tight coupling)
    // auto y_full_ini = set_ic_after_tight_coupling(y_tight_coupling, x_end_tight, k);
    auto y_full_ini = set_ic_after_tight_coupling(y_tight_coupling_ini, x_end_tight, k);

    // The full ODE system
    ODEFunction dydx_full = [&](double x, const double *y, double *dydx){
      return rhs_full_ode(x, k, y, dydx);
    };

    // Integrate from x_end_tight -> x_end
    // ...
    // ...
    // ...
    // ...
    // ...
    // Setting up ODE for after tightly coupled regime
    ODESolver ode_full;

    // Solving ODE and extracting solution array from ODEsolver
    ode_full.solve(dydx_full, x_full, y_full_ini);
    auto all_data_full = ode_full.get_data();
    
    x_full_index = 0;
    for (int jx = len_tc; jx < n_x; jx++){
      Phi[jx + n_x * ik]        = all_data_full[x_full_index][Constants.ind_Phi];
      delta_cdm[jx + n_x * ik]  = all_data_full[x_full_index][Constants.ind_deltacdm];
      delta_b[jx + n_x * ik]    = all_data_full[x_full_index][Constants.ind_deltab];
      v_cdm[jx + n_x * ik]      = all_data_full[x_full_index][Constants.ind_vcdm];
      v_b[jx + n_x * ik]        = all_data_full[x_full_index][Constants.ind_vb];
        
      for (int ell = 0; ell < Constants.n_ell_theta; ell++){
          Thetas[ell][jx + n_x * ik] = all_data_full[x_full_index][Constants.ind_start_theta + ell];
        }

      Psi[jx + n_x * ik] = - Phi[jx + n_x * ik] 
                           - 12.0 * H0 * H0 / (c * c * k * k * exp(2 * x_full[x_full_index]))
                           * OmegaR0 * Thetas[2][jx + n_x * ik];
      x_full_index++;
    }
    
    //===================================================================
    // TODO: remember to store the data found from integrating so we can
    // spline it below
    //
    // To compute a 2D spline of a function f(x,k) the data must be given 
    // to the spline routine as a 1D array f_array with the points f(ix, ik) 
    // stored as f_array[ix + n_x * ik]
    // Example:
    // Vector x_array(n_x);
    // Vector k_array(n_k);
    // Vector f(n_x * n_k);
    // Spline2D y_spline;
    // f_spline.create(x_array, k_array, f_array);
    // We can now use the spline as f_spline(x, k)
    //
    //===================================================================
    //...
    //...
  }
  Utils::StartTiming("integrateperturbation");

  //=============================================================================
  // TODO: Make all splines needed: Theta0,Theta1,Theta2,Phi,Psi,...
  //=============================================================================
  // ...
  // ...
  // ...
  delta_cdm_spline.create(x_all, k_array, delta_cdm, "delta_cdm spline");
  delta_b_spline.create(x_all, k_array, delta_b, "delta_b spline");
  v_cdm_spline.create(x_all, k_array, v_cdm, "v_cdm spline");
  v_b_spline.create(x_all, k_array, v_b, "v_b spline");
  Phi_spline.create(x_all, k_array, Phi, "Phi spline");
  Psi_spline.create(x_all, k_array, Psi, "Psi spline");
  for (int ell = 0; ell < Constants.n_ell_theta; ell++){
    Theta_element_spline.create(x_all, k_array, Thetas[ell], "Theta elements");
    Theta_spline.push_back(Theta_element_spline);
  }
  std::cout << "hej " << x_all[n_x - 1] << std::endl;
}

//====================================================
// Set IC at the start of the run (this is in the
// tight coupling regime)
//====================================================
Vector Perturbations::set_ic(const double x, const double k) const{

  // The vector we are going to fill
  Vector y_tc(Constants.n_ell_tot_tc);

  //=============================================================================
  // Compute where in the y_tc array each component belongs
  // This is just an example of how to do it to make it easier
  // Feel free to organize the component any way you like
  //=============================================================================
  
  // For integration of perturbations in tight coupling regime (Only 2 photon multipoles + neutrinos needed)
  const int n_ell_theta_tc      = Constants.n_ell_theta_tc;
  const int n_ell_neutrinos_tc  = Constants.n_ell_neutrinos_tc;
  const int n_ell_tot_tc        = Constants.n_ell_tot_tc;
  const bool polarization       = Constants.polarization;
  const bool neutrinos          = Constants.neutrinos;

  // References to the tight coupling quantities
  double &delta_cdm    =  y_tc[Constants.ind_deltacdm_tc];
  double &delta_b      =  y_tc[Constants.ind_deltab_tc];
  double &v_cdm        =  y_tc[Constants.ind_vcdm_tc];
  double &v_b          =  y_tc[Constants.ind_vb_tc];
  double &Phi          =  y_tc[Constants.ind_Phi_tc];
  double *Theta        = &y_tc[Constants.ind_start_theta_tc];

  //=============================================================================
  // TODO: Set the initial conditions in the tight coupling regime
  //=============================================================================
  // ...
  // ...

  // SET: Scalar quantities (Gravitational potential, baryons and CDM)
  // ...
  // ...
  double Hp = cosmo -> Hp_of_x(x);
  double dtaudx = rec -> dtaudx_of_x(x);

  double Psi_init = - 2.0 / 3.0;
  Phi             = - Psi_init;
  delta_cdm       = - 3.0 / 2.0 * Psi_init;
  delta_b         = delta_cdm;
  v_cdm           = - Constants.c * k / (2.0 * Hp) * Psi_init;
  v_b             = v_cdm;
  // SET: Photon temperature perturbations (Theta_ell)
  // ...
  // ...
  Theta[0] = -0.5 * Psi_init;
  Theta[1] = Constants.c * k / (6.0 * Hp) * Psi_init;
  return y_tc;
}

//====================================================
// Set IC for the full ODE system after tight coupling 
// regime ends
//====================================================

Vector Perturbations::set_ic_after_tight_coupling(
    const Vector &y_tc, 
    const double x, 
    const double k) const{

  // Make the vector we are going to fill
  Vector y(Constants.n_ell_tot_full);
  
  //=============================================================================
  // Compute where in the y array each component belongs and where corresponding
  // components are located in the y_tc array
  // This is just an example of how to do it to make it easier
  // Feel free to organize the component any way you like
  //=============================================================================

  // Number of multipoles we have in the full regime
  const int n_ell_theta         = Constants.n_ell_theta;

  // Number of multipoles we have in the tight coupling regime
  const int n_ell_theta_tc      = Constants.n_ell_theta_tc;
  const int n_ell_neutrinos_tc  = Constants.n_ell_neutrinos_tc;

  // References to the tight coupling quantities
  const double &delta_cdm_tc    =  y_tc[Constants.ind_deltacdm_tc];
  const double &delta_b_tc      =  y_tc[Constants.ind_deltab_tc];
  const double &v_cdm_tc        =  y_tc[Constants.ind_vcdm_tc];
  const double &v_b_tc          =  y_tc[Constants.ind_vb_tc];
  const double &Phi_tc          =  y_tc[Constants.ind_Phi_tc];
  const double *Theta_tc        = &y_tc[Constants.ind_start_theta_tc];

  // References to the quantities we are going to set
  double &delta_cdm       =  y[Constants.ind_deltacdm_tc];
  double &delta_b         =  y[Constants.ind_deltab_tc];
  double &v_cdm           =  y[Constants.ind_vcdm_tc];
  double &v_b             =  y[Constants.ind_vb_tc];
  double &Phi             =  y[Constants.ind_Phi_tc];
  double *Theta           = &y[Constants.ind_start_theta_tc];

  //=============================================================================
  // TODO: fill in the initial conditions for the full equation system below
  // NB: remember that we have different number of multipoles in the two
  // regimes so be careful when assigning from the tc array
  //=============================================================================
  // ...
  // ...
  // ...
  double Hp = cosmo -> Hp_of_x(x);
  double dtaudx = rec -> dtaudx_of_x(x);

  // SET: Scalar quantities (Gravitational potental, baryons and CDM)
  // ...
  // ...
  Phi       = Phi_tc;
  delta_cdm = delta_cdm_tc;
  delta_b   = delta_b_tc;
  v_cdm     = v_cdm_tc;
  v_b       = v_b_tc;
  // SET: Photon temperature perturbations (Theta_ell)
  // ...
  // ...
  Theta[0] = Theta_tc[0];
  Theta[1] = Theta_tc[1];
  Theta[2] = - 20.0 * Constants.c * k / (45.0 * Hp * dtaudx);
  
  
  for (int ell = 3; ell < Constants.n_ell_theta; ell++){
    Theta[ell] = - ell / (2.0 * ell + 1.0) * Constants.c * k / (Hp * dtaudx) * Theta[ell - 1];
  }
  
  return y;
}

//====================================================
// The time when tight coupling end
//====================================================

double Perturbations::get_tight_coupling_time(const double k) const{
  double x_tight_coupling_end;

  //=============================================================================
  // TODO: compute and return x for when tight coupling ends
  // Remember all the three conditions in Callin
  //=============================================================================
  // ...
  // ...
  double Xe;
  double c = Constants.c;
  double dtaudx;
  double Hp;
  int points = 5e3;
  Vector x = Utils::linspace(x_start, x_end, points);
  for (int i = 0; i < points; i++){
    Xe = rec -> Xe_of_x(x[i]);
    dtaudx = rec -> dtaudx_of_x(x[i]);
    Hp    = cosmo -> Hp_of_x(x[i]);
    if (std::fabs(dtaudx) < 10 * std::min(1.0, c * k / Hp)){
      x_tight_coupling_end = x[i];
      break;
    }
    if (Xe < 0.999){
      x_tight_coupling_end = x[i];
      break; 
    }
  }
  return x_tight_coupling_end;
}

//====================================================
// After integrsating the perturbation compute the
// source function(s)
//====================================================
void Perturbations::compute_source_functions(){
  Utils::StartTiming("source");

  //=============================================================================
  // TODO: Make the x and k arrays to evaluate over and use to make the splines
  //=============================================================================
  // ...
  // ...
  Vector k_array;
  Vector x_array;

  // Make storage for the source functions (in 1D array to be able to pass it to the spline)
  Vector ST_array(k_array.size() * x_array.size());
  Vector SE_array(k_array.size() * x_array.size());

  // Compute source functions
  for(auto ix = 0; ix < x_array.size(); ix++){
    const double x = x_array[ix];
    for(auto ik = 0; ik < k_array.size(); ik++){
      const double k = k_array[ik];

      // NB: This is the format the data needs to be stored 
      // in a 1D array for the 2D spline routine source(ix,ik) -> S_array[ix + nx * ik]
      const int index = ix + n_x * ik;

      //=============================================================================
      // TODO: Compute the source functions
      //=============================================================================
      // Fetch all the things we need...
      // const double Hp       = cosmo->Hp_of_x(x);
      // const double tau      = rec->tau_of_x(x);
      // ...
      // ...

      // Temperatur source
      ST_array[index] = 0.0;

      // Polarization source
      if(Constants.polarization){
        SE_array[index] = 0.0;
      }
    }
  }

  // Spline the source functions
  ST_spline.create (x_array, k_array, ST_array, "Source_Temp_x_k");
  if(Constants.polarization){
    SE_spline.create (x_array, k_array, SE_array, "Source_Pol_x_k");
  }

  Utils::EndTiming("source");
}

//====================================================
// The right hand side of the perturbations ODE
// in the tight coupling regime
//====================================================

// Derivatives in the tight coupling regime
int Perturbations::rhs_tight_coupling_ode(double x, double k, const double *y, double *dydx){

  //=============================================================================
  // Compute where in the y / dydx array each component belongs
  // This is just an example of how to do it to make it easier
  // Feel free to organize the component any way you like
  //=============================================================================
  
  // For integration of perturbations in tight coupling regime (Only 2 photon multipoles + neutrinos needed)
  const int n_ell_theta_tc      = Constants.n_ell_theta_tc;

  // The different quantities in the y array
  const double &delta_cdm       =  y[Constants.ind_deltacdm_tc];
  const double &delta_b         =  y[Constants.ind_deltab_tc];
  const double &v_cdm           =  y[Constants.ind_vcdm_tc];
  const double &v_b             =  y[Constants.ind_vb_tc];
  const double &Phi             =  y[Constants.ind_Phi_tc];
  const double *Theta           = &y[Constants.ind_start_theta_tc];

  // References to the quantities we are going to set in the dydx array
  double &ddelta_cdmdx    =  dydx[Constants.ind_deltacdm_tc];
  double &ddelta_bdx      =  dydx[Constants.ind_deltab_tc];
  double &dv_cdmdx        =  dydx[Constants.ind_vcdm_tc];
  double &dv_bdx          =  dydx[Constants.ind_vb_tc];
  double &dPhidx          =  dydx[Constants.ind_Phi_tc];
  double *dThetadx        = &dydx[Constants.ind_start_theta_tc];

  //=============================================================================
  // TODO: fill in the expressions for all the derivatives
  //=============================================================================
  const double H0             = cosmo -> get_H0();
  const double Hp             = cosmo -> Hp_of_x(x);
  const double dHpdx          = cosmo -> dHpdx_of_x(x);
  const double c              = Constants.c; 
  // Often used combination of constants
  const double ckHp           = c * k / Hp;
  const double OmegaR0        = cosmo -> get_OmegaR(0);
  const double OmegaCDM0      = cosmo -> get_OmegaCDM(0);
  const double OmegaB0        = cosmo -> get_OmegaB(0);
  const double OmegaLambda0   = cosmo -> get_OmegaLambda(0);
  const double dtaudx         = rec -> dtaudx_of_x(x);
  const double ddtauddx       = rec -> ddtauddx_of_x(x);
  const double R              = 4.0 * OmegaR0 / (3.0 * OmegaB0) * exp(- x);
  double q;

  // SET: Scalar quantities (Phi, delta, v, ...)
  // ...
  // ...
  // ...

  double Theta2 = - 20.0 * ckHp / (45.0 * dtaudx) * Theta[1];
  double Psi = - Phi - 12.0 * H0 * H0 / (c * c * k * k * exp(2 * x)) 
                     * OmegaR0 * Theta2;
  
  dPhidx = Psi - c * c * k * k / (3.0 * Hp * Hp) * Phi
               + H0 * H0 / (2.0 * Hp * Hp) 
               * (OmegaCDM0 * exp(-x) * delta_cdm
               +  OmegaB0 * exp(-x) * delta_b
               + 4 * OmegaR0 * exp(- 2 * x) * Theta[0]);
  
  ddelta_cdmdx = ckHp * v_cdm - 3 * dPhidx;
  dv_cdmdx     = - v_cdm - ckHp * Psi;
  ddelta_bdx   = ckHp * v_b - 3 * dPhidx;



  // SET: Photon multipoles (Theta_ell)
  // ...
  // ...
  dThetadx[0]  = - ckHp * Theta[1] - dPhidx;
  q            = - ((1 - R) * dtaudx + (1 + R) * ddtauddx) * (3 * Theta[1] + v_b)
                - ckHp * Psi
                + ckHp * (1 - dHpdx / Hp) * (-Theta[0] + 2 * Theta2) 
                - ckHp * dThetadx[0];
  q           /= (1 + R) * dtaudx + dHpdx / Hp - 1;
  dv_bdx       = (- v_b - ckHp * Psi + R * (q + ckHp * (-Theta[0] + 2 * Theta2) - ckHp * Psi))
                  / (1 + R);
  dThetadx[1]  = (q - dv_bdx) / 3.0;

  return GSL_SUCCESS;
}

//====================================================
// The right hand side of the full ODE
//====================================================

int Perturbations::rhs_full_ode(double x, double k, const double *y, double *dydx){
  
  //=============================================================================
  // Compute where in the y / dydx array each component belongs
  // This is just an example of how to do it to make it easier
  // Feel free to organize the component any way you like
  //=============================================================================

  // Index and number of the different quantities
  const int n_ell_theta         = Constants.n_ell_theta;
  const int n_ell_thetap        = Constants.n_ell_thetap;
  const int n_ell_neutrinos     = Constants.n_ell_neutrinos;
  const bool polarization       = Constants.polarization;
  const bool neutrinos          = Constants.neutrinos;

  // The different quantities in the y array
  const double &delta_cdm       =  y[Constants.ind_deltacdm];
  const double &delta_b         =  y[Constants.ind_deltab];
  const double &v_cdm           =  y[Constants.ind_vcdm];
  const double &v_b             =  y[Constants.ind_vb];
  const double &Phi             =  y[Constants.ind_Phi];
  const double *Theta           = &y[Constants.ind_start_theta];
  const double *Theta_p         = &y[Constants.ind_start_thetap];
  const double *Nu              = &y[Constants.ind_start_nu];

  // References to the quantities we are going to set in the dydx array
  double &ddelta_cdmdx    =  dydx[Constants.ind_deltacdm];
  double &ddelta_bdx      =  dydx[Constants.ind_deltab];
  double &dv_cdmdx        =  dydx[Constants.ind_vcdm];
  double &dv_bdx          =  dydx[Constants.ind_vb];
  double &dPhidx          =  dydx[Constants.ind_Phi];
  double *dThetadx        = &dydx[Constants.ind_start_theta];
  double *dTheta_pdx      = &dydx[Constants.ind_start_thetap];
  double *dNudx           = &dydx[Constants.ind_start_nu];

  // Cosmological parameters and variables
  // double Hp = cosmo->Hp_of_x(x);
  // ...
  const double H0             = cosmo -> get_H0();
  const double Hp             = cosmo -> Hp_of_x(x);
  const double dHpdx          = cosmo -> dHpdx_of_x(x);
  const double c              = Constants.c; 
  // Often used combination of constants
  const double ckHp           = c * k / Hp;
  const double OmegaR0        = cosmo -> get_OmegaR(0);
  const double OmegaCDM0      = cosmo -> get_OmegaCDM(0);
  const double OmegaB0        = cosmo -> get_OmegaB(0);
  const double OmegaLambda0   = cosmo -> get_OmegaLambda(0);
  const double dtaudx         = rec -> dtaudx_of_x(x);
  const double ddtauddx       = rec -> ddtauddx_of_x(x);
  const double R              = 4.0 * OmegaR0 / (3.0 * OmegaB0) * exp(- x);
  const double eta            = cosmo -> eta_of_x(x);
  const int ell_max           = Constants.n_ell_theta_tc - 1;
  double q;

  // Recombination variables
  // ...

  //=============================================================================
  // TODO: fill in the expressions for all the derivatives
  //=============================================================================

  // SET: Scalar quantities (Phi, delta, v, ...)
  // ...
  // ...
  // ...
  double Psi = - Phi - 12.0 * H0 * H0 / (c * c * k * k * exp(2 * x)) 
                     * OmegaR0 * Theta[2];
  
  dPhidx = Psi - c * c * k * k / (3 * Hp * Hp) * Phi
               + H0 * H0 / (2 * Hp * Hp) 
               * (OmegaCDM0 * exp(-x) * delta_cdm
               +  OmegaB0 * exp(-x) * delta_b
               + 4 * OmegaR0 * exp(- 2 * x) * Theta[0]);
  
  ddelta_cdmdx = ckHp * v_cdm - 3 * dPhidx;
  dv_cdmdx     = - v_cdm - ckHp * Psi;
  ddelta_bdx   = ckHp * v_b - 3 * dPhidx;
  dv_bdx       = - v_b - ckHp * Psi + R * dtaudx * (3.0 * Theta[1] + v_b);

  // SET: Photon multipoles (Theta_ell)
  // ...
  // ...
  // ...
  dThetadx[0]  = - ckHp * Theta[1] - dPhidx;
  dThetadx[1]  =  ckHp / 3.0 * Theta[0] 
                  - 2.0 / 3.0 * ckHp * Theta[2] 
                  + ckHp / 3.0 * Psi 
                  + dtaudx * (Theta[1] + v_b / 3.0);

  dThetadx[2]  =  2.0 / 5.0 * ckHp * Theta[1] 
                - 3.0 / 5.0 * ckHp * Theta[3] + dtaudx * (Theta[2] 
                - 1.0 / 10.0 * Theta[2]);

  for (int ell = 3; ell < Constants.n_ell_theta_tc - 1; ell++){   
    dThetadx[ell] =   ell / (2.0 * ell + 1.0) * ckHp * Theta[ell - 1]
                    - (ell + 1.0) / (2.0 * ell + 1.0) * ckHp * Theta[ell + 1]
                    + dtaudx * Theta[ell]; 
  }
  
  dThetadx[ell_max] = ckHp * Theta[ell_max - 1]
                    - (ell_max + 1.0) / (Hp * eta) * c * Theta[ell_max]
                    + dtaudx * Theta[ell_max];

  return GSL_SUCCESS;
}

//====================================================
// Get methods
//====================================================

double Perturbations::get_delta_cdm(const double x, const double k) const{
  return delta_cdm_spline(x, k);
}
double Perturbations::get_delta_b(const double x, const double k) const{
  return delta_b_spline(x, k);
}
double Perturbations::get_v_cdm(const double x, const double k) const{
  return v_cdm_spline(x, k);
}
double Perturbations::get_v_b(const double x, const double k) const{
  return v_b_spline(x, k);
}
double Perturbations::get_Phi(const double x, const double k) const{
  return Phi_spline(x, k);
}
double Perturbations::get_Psi(const double x, const double k) const{
  return Psi_spline(x, k);
}
double Perturbations::get_Pi(const double x, const double k) const{
  return Pi_spline(x, k);
}
double Perturbations::get_Source_T(const double x, const double k) const{
  return ST_spline(x, k);
}
double Perturbations::get_Source_E(const double x, const double k) const{
  return SE_spline(x, k);
}
double Perturbations::get_Theta(const double x, const double k, const int ell) const{
  return Theta_spline[ell](x, k);
}
double Perturbations::get_Theta_p(const double x, const double k, const int ell) const{
  return Theta_p_spline[ell](x, k);
}
double Perturbations::get_Nu(const double x, const double k, const int ell) const{
  return Nu_spline[ell](x,k);
}

//====================================================
// Print some useful info about the class
//====================================================

void Perturbations::info() const{
  std::cout << "\n";
  std::cout << "Info about perturbations class:\n";
  std::cout << "x_start:       " << x_start                << "\n";
  std::cout << "x_end:         " << x_end                  << "\n";
  std::cout << "n_x:     " << n_x              << "\n";
  std::cout << "k_min (1/Mpc): " << k_min * Constants.Mpc  << "\n";
  std::cout << "k_max (1/Mpc): " << k_max * Constants.Mpc  << "\n";
  std::cout << "n_k:     " << n_k              << "\n";
  if(Constants.polarization)
    std::cout << "We include polarization\n";
  else
    std::cout << "We do not include polarization\n";
  if(Constants.neutrinos)
    std::cout << "We include neutrinos\n";
  else
    std::cout << "We do not include neutrinos\n";

  std::cout << "Information about the perturbation system:\n";
  std::cout << "ind_deltacdm:       " << Constants.ind_deltacdm         << "\n";
  std::cout << "ind_deltab:         " << Constants.ind_deltab           << "\n";
  std::cout << "ind_v_cdm:          " << Constants.ind_vcdm             << "\n";
  std::cout << "ind_v_b:            " << Constants.ind_vb               << "\n";
  std::cout << "ind_Phi:            " << Constants.ind_Phi              << "\n";
  std::cout << "ind_start_theta:    " << Constants.ind_start_theta      << "\n";
  std::cout << "n_ell_theta:        " << Constants.n_ell_theta          << "\n";
  if(Constants.polarization){
    std::cout << "ind_start_thetap:   " << Constants.ind_start_thetap   << "\n";
    std::cout << "n_ell_thetap:       " << Constants.n_ell_thetap       << "\n";
  }
  if(Constants.neutrinos){
    std::cout << "ind_start_nu:       " << Constants.ind_start_nu       << "\n";
    std::cout << "n_ell_neutrinos     " << Constants.n_ell_neutrinos    << "\n";
  }
  std::cout << "n_ell_tot_full:     " << Constants.n_ell_tot_full       << "\n";

  std::cout << "Information about the perturbation system in tight coupling:\n";
  std::cout << "ind_deltacdm:       " << Constants.ind_deltacdm_tc      << "\n";
  std::cout << "ind_deltab:         " << Constants.ind_deltab_tc        << "\n";
  std::cout << "ind_v_cdm:          " << Constants.ind_vcdm_tc          << "\n";
  std::cout << "ind_v_b:            " << Constants.ind_vb_tc            << "\n";
  std::cout << "ind_Phi:            " << Constants.ind_Phi_tc           << "\n";
  std::cout << "ind_start_theta:    " << Constants.ind_start_theta_tc   << "\n";
  std::cout << "n_ell_theta:        " << Constants.n_ell_theta_tc       << "\n";
  if(Constants.neutrinos){
    std::cout << "ind_start_nu:       " << Constants.ind_start_nu_tc    << "\n";
    std::cout << "n_ell_neutrinos     " << Constants.n_ell_neutrinos_tc << "\n";
  }
  std::cout << "n_ell_tot_tc:       " << Constants.n_ell_tot_tc         << "\n";
  std::cout << std::endl;
}

//====================================================
// Output some results to file for a given value of k
//====================================================

void Perturbations::output(const double k, const std::string filename) const{
  std::ofstream fp(filename.c_str());
  const int npts = 5000;

  auto x_array = Utils::linspace(x_start, x_end, npts);
  auto print_data = [&] (const double x) {
    double arg = k * Constants.c * (cosmo -> eta_of_x(0.0) - cosmo -> eta_of_x(x));
    fp << x                  << " ";
    fp << get_Theta(x, k, 0)   << " ";
    fp << get_Theta(x, k, 1)   << " ";
    fp << get_Theta(x, k, 2)   << " ";
    fp << get_Phi(x, k)       << " ";
    fp << get_Psi(x, k)       << " ";
    //fp << get_Pi(x, k)        << " ";
    //fp << get_Source_T(x,k)  << " ";
    //fp << get_Source_T(x,k) * Utils::j_ell(5,   arg)           << " ";
    //fp << get_Source_T(x,k) * Utils::j_ell(50,  arg)           << " ";
    //fp << get_Source_T(x,k) * Utils::j_ell(500, arg)           << " ";
    fp << "\n";
  };
  std::for_each(x_array.begin(), x_array.end(), print_data);
}

