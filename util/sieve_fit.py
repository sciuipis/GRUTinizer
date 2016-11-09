#!/usr/bin/python
import numpy as np
from scipy import interpolate
import argparse
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
# http://stackoverflow.com/questions/16333296/how-do-you-create-nested-dict-in-python
import collections
# http://matplotlib.org/examples/pylab_examples/demo_tight_layout.html
import matplotlib.gridspec as gridspec
# https://docs.scipy.org/doc/scipy-0.18.1/reference/generated/scipy.optimize.curve_fit.html
from scipy.optimize import curve_fit


def Bfit(var,c000,c001,c010,c011,c100,c101,c110,c111):
    x,a,y = var
    return c000+c001*y+c010*a+c011*y*a+c100*x+c101*y*x+c110*a*x+c111*x*a*y

# def bfit(var,*c):
#     x,a,y = var
#     return c[0]+c[1]*y+c[2]*a+c[3]*y*a+c[4]*x+c[5]*y*x+c[6]*a*x+c[7]*x*a*y


def Afit(var,a0,a1,a2):
    x,a = var
    return a0+a1*x+a2*a

class SieveSlitFit(object):
    def __init__(self,filepath):
        self.drawing_holes = []
        self.data_holes = []
        self.A = []
        self.B = []
        self.x = []
        self.parse(filepath)
        self.degree_y=1
        #SieveSliteFit.global_degree=3

    def parse(self,filepath):
        B = set()
        A = set()
        x = set()

        drawing = False
        data = False
        for line in open(filepath):
            elements = line.split()
            if len(elements) == 0:
                drawing = False
                data = False
                continue
            if '#' in line:
                continue
            if 'Drawing' in line:
                drawing = True
                continue
            if 'Data' in line:
                data = True
                continue
            if drawing:
                row = [float(k) for k in elements]
                self.drawing_holes.append(row)
                A.add(row[0])
                B.add(row[1])

            if data:
                row = [float(k) for k in elements]
                self.data_holes.append(row)
                x.add(row[0])
        self.drawing_holes = np.asarray(self.drawing_holes)
        self.data_holes = np.asarray(self.data_holes)
        self.A = sorted(A)
        self.B = sorted(B)
        self.x = sorted(x)

    def plot_y_dependence(self):
        self.fit_params = [[]]*len(self.x)
        for i,x in enumerate(self.x):
            scatter_a = []
            scatter_y = []
            for row in self.data_holes:
                if row[0] == x:
                    scatter_a.append(row[1])
                    scatter_y.append(row[2])
            a_prev = scatter_a[0]
            ys = []
            for j,a in enumerate(scatter_a):
                if a != a_prev or j == len(scatter_a)-1:
                    if j == len(scatter_a)-1:
                        ys.append(scatter_y[j])
                    plt.plot(ys,self.B)
                    params = np.polyfit(ys,self.B,self.degree_y)
                    self.fit_params[i].append(params)
                    plt.plot(ys,[sum([params[n]*pow(y,len(params)-n-1) for n in range(0,len(params))]) for y in ys],'r--')
                    plt.show()
                    ys = []
                    a_prev = a
                ys.append(scatter_y[j])


            plt.scatter(scatter_a,scatter_y)
            plt.show()

    def fit_y_dependence(self):
        #self.fit_params = []
        #self.fit_params = dict()
        self.fit_params = collections.defaultdict(dict)

        xpt = []
        apt = []
        for i,x in enumerate(self.x):
            scatter_a = []
            scatter_y = []
            for row in self.data_holes:
                if row[0] == x:
                    scatter_a.append(row[1])
                    scatter_y.append(row[2])
            a_prev = scatter_a[0]
            ys = []
            for j,a in enumerate(scatter_a):
                # many y's for a given a, only fit when a changes
                if a != a_prev or j == len(scatter_a)-1:
                    # also fit the last item in the list
                    if j == len(scatter_a)-1:
                        ys.append(scatter_y[j])
                    params = np.polyfit(ys,self.B,self.degree_y)
                    self.fit_params[x][a_prev] = params
                    ys = []
                    a_prev = a
                ys.append(scatter_y[j])
        apt = sorted(apt)
        pts = set()
        for row in self.data_holes:
            pts.add((row[0],row[1]))
            #xpt.append(row[0])
            #apt.append(row[1])
        data = [list(t) for t in zip(*sorted(pts))]
        params = []
        for i,x in enumerate(data[0]):
            a = data[1][i]
            params.append(self.fit_params[x][a])
        params = np.asarray(params)

        # interpolate data
        interpolant = interpolate.interp2d(data[0],data[1],params[:,0])
        uniquex_interp = np.arange(-600,600,100)
        uniquea_interp = np.arange(-0.1,0.1,0.015)
        uniquea_interp = uniquea_interp[:len(uniquex_interp)]
        domainx = []
        domaina = []
        for x in uniquex_interp:
            for a in uniquea_interp:
                domainx.append(x)
                domaina.append(a)

        print len(domainx),len(domaina)
        rangez = [interpolant(x,domaina[i])[0] for i,x in enumerate(domainx)]
        #print rangez


        fig = plt.figure()
        gs1 = gridspec.GridSpec(1, self.degree_y+1)
        for i in range(0,self.degree_y+1):
            ax = fig.add_subplot(gs1[i], projection='3d')
            ax.scatter(data[0],data[1],params[:,i],c='red')
            #ax.scatter(domainx,domaina,rangez)
        plt.show()

    def prepare_global_fit_data(self):
        xvals = []
        avals = []
        yvals = []
        Bvals = []
        for i,row in enumerate(self.data_holes):
            xvals.append(row[0])
            avals.append(row[1])
            yvals.append(row[2])
            Bvals.append(self.B[i%5])
        return xvals,avals,yvals,Bvals

    def global_fit(self):
        xvals,avals,yvals,Bvals = self.prepare_global_fit_data()
        data = (xvals,avals,yvals)
        popt,pcov = curve_fit(bfit,data,Bvals,p0=[1]*pow(SieveSlitFit.global_degree+1,len(data)))
        print(popt)
        def local_b_fit(x,a,y):
            return bfit([x,a,y],*popt)
        self.fit = local_b_fit
        print
        for row in self.data_holes:
            X=row[:3]
            print X,bfit(X,*popt)

    def plot_global_fit(self):
        for i,x in enumerate(self.x):
            scatter_a = []
            scatter_y = []
            scatter_B = []
            for row in self.data_holes:
                if row[0] == x:
                    scatter_a.append(row[1])
                    scatter_y.append(row[2])
                    scatter_B.append(self.fit(row[0],row[1],row[2]))
            plt.scatter(scatter_a,scatter_y,marker='+')
            plt.scatter(scatter_a,scatter_B,c='red')
            plt.show()

def bfit(var,*c):
    x,a,y = var
    total = 0
    counter = 0
    for i in range(0,SieveSlitFit.global_degree+1):
        sum1 = 0
        for j in range(0,SieveSlitFit.global_degree+1):
            sum2 = 0
            for k in range(0,SieveSlitFit.global_degree+1):
                sum2 += c[counter]*pow(y,k)
                counter+=1
            sum1+=sum2*pow(a,j)
        total+=sum1*pow(x,i)
    return total


if __name__=="__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str,help="Input sieve slit data",default=None)
    args = parser.parse_args()

    if not args.input:
        parser.error("No input file specified.")

    fitter = SieveSlitFit(args.input)
    #fitter.degree_y = 1
    #fitter.fit_y_dependence()
    SieveSlitFit.global_degree = 2
    fitter.global_fit()
    fitter.plot_global_fit()
