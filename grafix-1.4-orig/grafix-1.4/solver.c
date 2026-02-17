#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define f(psi,psip,uu) ((uu)*(((uu)>0.)?(psi):(psip)))

int nx;
double *u, *q;
double dx, dt;

// general advection scheme -> for Smolarkievicz && simple upstream 
// Smolarkievicz method : set SC = 1.08
// simple upstream : SC = 0.0
// used globals are : u, q, dt, dx, nx

void general(int mi1, int mni1, double SC) 
{
  double EPS = 1.e-15;
  int i,mi2,mni2,im1,ip1;
  double qq[nx];
  double u0,upl[nx],umi[nx],uplsn,umisn;
  double t0,t1,t2,hilfx;
  
  hilfx =.5*dt/dx;
  mi2 = mi1+1; mni2 = mni1-1;
  for(i = mi2; i < mni2; i++) {
    im1 = i-1; ip1 = i+1; u0 = u[i];
    uplsn=(u0 + u[ip1])*hilfx;
    umisn=(u0 + u[im1])*hilfx;

    upl[i]= fabs(uplsn) - uplsn*uplsn;
    umi[i]= fabs(umisn) - umisn*umisn;
    
    if(uplsn > 0.) {
      if(umisn > 0.) {
	
	t0 = q[i];
	qq[i] = t0 + umisn*(q[im1] - t0);
      } else qq[i] = q[i];
    } else {
      if (umisn > 0.) {
	t0 = q[i];
	qq[i] = t0 - uplsn*(q[ip1] - t0) + umisn*(q[im1] - t0);
      } else {
	t0 = q[i];
	qq[i] = t0 - uplsn*(q[ip1] - t0);
      }
    }
  }
 
  qq[mi1]=q[mi1];
  qq[mni2]=q[mni2];

  for(i=mi2;i<mni2;i++) {
    t0=qq[i]; t1=qq[i-1]; t2=qq[i+1];

    uplsn=upl[i] * (t2 - t0) / (t0+t2+EPS);
    umisn=umi[i] * (t0 - t1) / (t1+t0+EPS);
    q[i] = t0 + SC*(  - f(t0,t2,uplsn) + f(t1,t0,umisn) );
  }
}
// general advection scheme -> for Smolarkievicz && simple upstream 
// Smolarkievicz method : set SC = 1.08
// simple upstream : SC = 0.0
// used globals are : u, q, dt, dx, nx
// !! simplified : only for case u > 0 !!
// it uses zyklical boundary conditions : x[0] == x[nx]
void general2(int mi1, int mni1, double SC) 
{
  double EPS = 1.e-15;
  int i,im1,ip1;
  double qq[nx];
  double u0,upl[nx],umi[nx],uplsn,umisn;
  double t0,t1,t2,hilfx;
  
  hilfx =.5*dt/dx;

  for(i = mi1; i < mni1; i++) {
    im1 = i-1; if (im1 < 0) im1= nx-1; // wrap around at i == 0 && i == nx-1
    ip1 = i+1; if (ip1 == nx) ip1 = 0;
    u0 = u[i];
    uplsn=(u0 + u[ip1])*hilfx;
    umisn=(u0 + u[im1])*hilfx;

    upl[i]= fabs(uplsn) - uplsn*uplsn;
    umi[i]= fabs(umisn) - umisn*umisn;
    
	
    t0 = q[i];
    qq[i] = t0 + umisn*(q[im1] - t0);
  }
 
  for(i=mi1; i< mni1; i++) { 
    im1 = i-1; if (im1 < 0) im1= nx-1; // wrap around at i == 0 && i == nx-1
    ip1 = i+1; if (ip1 == nx) ip1 = 0;
  
    t0=qq[i]; t1=qq[im1]; t2=qq[ip1];

    uplsn=upl[i] * (t2 - t0) / (t0+t2+EPS);
    umisn=umi[i] * (t0 - t1) / (t1+t0+EPS);
    q[i] = t0 + SC*(  - f(t0,t2,uplsn) + f(t1,t0,umisn) );
  }
}

// globals : u,q,dx,dt,nx
void lax_wendroff(int x1,int x2)
{  double alpha, y,yp,ym;
   double qq[nx];
   int i,ip,im;
   for (i= x1; i < x2; i++)
     { alpha = u[i] * dt/dx; 
       // zyklischer Rand
       ip = (i < nx-1) ? (i+1) : 0; im = (i > 0) ? (i - 1) : (nx - 1); 
       y= q[i]; yp = q[ip];  ym = q[im];
       qq[i] = y - alpha/2 * (yp - ym - alpha * (yp + ym - 2.0*y)) ;
     }
   for (i=x1; i < x2; i++) q[i] = qq[i];
}

void smolarkievicz(int i, int j) { general2(i,j,1.08); }
void simple(int i, int j) { general2(i,j,0); }

void diffusion(double diff, int x1, int x2 ) 
{ int i, ip, im;
  double dq[nx], beta; 
  beta = diff * dt / (dx * dx);
  for (i=x1; i<x2; i++) 
    {  // zyklischer Rand
       ip = (i < nx-1) ? (i+1) : 0; im = (i > 0) ? (i - 1) : (nx - 1); 
       dq[i] = beta * (q[ip] - 2.0*q[i] + q[im]);
    }
  for (i=x1; i < x2; i++) q[i] += dq[i];
}
