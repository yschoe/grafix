#include "smolark.h"

int nq = NQ;

/* qadve1.c Smolarkievicz advection method
   from ~mieth/reich93/source/
   */ 
  
#define EPS 1.e-15
#define SC 1.08
  
#define f(psi,psip,uu) ((uu)*(((uu)>0.)?(psi):(psip)))

void qadve_1(float *qp, float *up, float *vp, float dt, int nx, int ny,
	     int mi1, int mni1, int mj1, int mnj1, float sc);

void smolark(float *qp, float *up, float *vp, float dt, int nx, int ny, 
	     int mi1, int mni1, int mj1, int mnj1)
{ qadve_1(qp, up, vp, dt, nx, ny, mi1, mni1, mj1, mnj1, SC); }

void simple(float *qp, float *up, float *vp, float dt, int nx, int ny, int mi1,
	    int mni1, int mj1, int mnj1)
{ qadve_1(qp, up, vp, dt, nx, ny, mi1, mni1, mj1, mnj1, 0); }

/* general smolarkievicz solver : 
   sc = 0 -> simple upstream
*/

void qadve_1(float *qp, float *up, float *vp, float dt, int nx, int ny,
	     int mi1, int mni1, int mj1, int mnj1, float sc)
{ float (*q)[ny][nq] = (float (*) [ny][nq]) qp;
  float (*u)[ny] = (float (*) [ny]) up;
  float (*v)[ny] = (float (*) [ny]) vp;

  float mx = 1.0, my = 1.0, dx = 1.0, dy = 1.0;
  int i,j,lq,mi2,mni2,mj2,mnj2,im1,jm1,ip1,jp1;
  float qq[nx][ny][nq];
  float u0,v0,upl[nx][ny],vpl[nx][ny],umi[nx][ny],vmi[nx][ny],uplsn,vplsn,umisn,vmisn;
  float t0,t1,t2,t3,t4,hilfx,hilfy;

  hilfx=.5*mx*dt/dx; hilfy=.5*my*dt/dy;
   
  mi2=mi1+1; mj2=mj1+1; mni2=mni1-1; mnj2=mnj1-1; 

  /* for(k=0;k<1;k++) { */ 
    for(i = mi2; i < mni2 ; i++) {
      im1 = i-1; ip1 = i+1; 
      for(j = mj2; j < mnj2; j++) { 
	jm1 = j-1; jp1 = j+1; 

	u0=u[i][j]; v0=v[i][j];
	uplsn=(u0+u[ip1][j])*hilfx;
	umisn=(u0+u[im1][j])*hilfx;
	vplsn=(v0+v[i][jp1])*hilfy;
	vmisn=(v0+v[i][jm1])*hilfy;
	if(uplsn>0.){
	  if(umisn>0.){
	    if(vplsn>0.){
	      if(vmisn>0.){
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(umisn*(q[im1][j][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+umisn*(-t0+q[im1][j][lq]);
		}
	      }
	    }
	    else{
	      if(vmisn>0.){
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+
		    (umisn*(q[im1][j][lq]-t0)-vplsn*(q[i][jp1][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(umisn*(q[im1][j][lq]-t0)-vplsn*(q[i][jp1][lq]-t0));
		}
	      }
	    }
	  }
	  else{
	    if(vplsn>0.){
	      if(vmisn>0.){
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+vmisn*(-t0+q[i][jm1][lq]);
		}
	      }
	      else{
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++) qq[i][j][lq]=q[i][j][lq];
	      }
	    }
	    else{
	      if(vmisn>0.){
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-vplsn*(q[i][jp1][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+vplsn*(t0-q[i][jp1][lq]);
		}
	      }
	    }
	  }
	}
	else{
	  if(umisn>0.){
	    if(vplsn>0.){
	      if(vmisn>0.){
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+
		    (-uplsn*(q[ip1][j][lq]-t0)+umisn*(q[im1][j][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-uplsn*(q[ip1][j][lq]-t0)+umisn*(q[im1][j][lq]-t0));
		}
	      }
	    }
	    else{
	      if(vmisn>0.){
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-uplsn*(q[ip1][j][lq]-t0)+umisn*(q[im1][j][lq]-t0)
				   -vplsn*(q[i][jp1][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+
		    (-uplsn*(q[ip1][j][lq]-t0)+umisn*(q[im1][j][lq]-t0)-vplsn*(q[i][jp1][lq]-t0));
		}
	      }
	    }
	  }
	  else{
	    if(vplsn>0.){
	      if(vmisn>0.){
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-uplsn*(q[ip1][j][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+uplsn*(t0-q[ip1][j][lq]);
		}
	      }
	    }
	    else{
	      if(vmisn>0.){
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-uplsn*(q[ip1][j][lq]-t0)
				   -vplsn*(q[i][jp1][lq]-t0)+vmisn*(q[i][jm1][lq]-t0));
		}
	      }
	      else{
		upl[i][j]=-uplsn-uplsn*uplsn;
		umi[i][j]=-umisn-umisn*umisn;
		vpl[i][j]=-vplsn-vplsn*vplsn;
		vmi[i][j]=-vmisn-vmisn*vmisn;
		for(lq=0;lq<nq;lq++){
		  t0=q[i][j][lq];
		  qq[i][j][lq]=t0+(-uplsn*(q[ip1][j][lq]-t0)-vplsn*(q[i][jp1][lq]-t0));
		}
	      }
	    }
	  }
	}
      }
    }

    for(lq=0;lq<nq;lq++){
      for(i=mi1;i<mni1;i++){
	qq[i][mj1][lq]=q[i][mj1][lq];
	qq[i][mnj2][lq]=q[i][mnj2][lq];
      }
      for (j=mj2;j<mnj2;j++) {
	qq[mi1][j][lq]=q[mi1][j][lq];
	qq[mni2][j][lq]=q[mni2][j][lq];
      }
    }

  for(i=mi2;i<mni2;i++) {
    im1=i-1; ip1=i+1;  
    for(j=mj2; j<mnj2; j++) {
      jm1=j-1; jp1=j+1; 
      
      for (lq=0;lq<nq;lq++) {
	  t0=qq[i][j][lq]; t1=qq[im1][j][lq];
	  t2=qq[ip1][j][lq]; t3=qq[i][jm1][lq]; t4=qq[i][jp1][lq];
	  uplsn=upl[i][j]*(t2-t0)/(t0+t2+EPS);
	  umisn=umi[i][j]*(t0-t1)/(t1+t0+EPS);
	  vplsn=vpl[i][j]*(t4-t0)/(t4+t0+EPS);
	  vmisn=vmi[i][j]*(t0-t3)/(t3+t0+EPS);
	  q[i][j][lq]=t0+sc*((-f(t0,t2,uplsn)+f(t1,t0,umisn))+(-f(t0,t4,vplsn)+f(t3,t0,vmisn)));
      }
    }
  }
}



 	
