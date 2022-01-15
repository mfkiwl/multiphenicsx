## multiphenicsx -- easy prototyping of multiphysics problems in FEniCSx ##
![multiphenicsx -- easy prototyping of multiphysics problems in FEniCSx](https://raw.githubusercontent.com/multiphenics/multiphenicsx/main/docs/multiphenicsx-logo-small.png "multiphenicsx -- easy prototyping of multiphysics problems in FEniCSx")

### 0. Introduction
**multiphenicsx** is a python library that aims at providing tools in **FEniCSx** for an easy prototyping of multiphysics problems on conforming meshes. In particular, it facilitates the definition of subdomain/boundary restricted variables.

### 1. Prerequisites
**multiphenicsx** requires **FEniCSx**.

### 2. Installation and usage
Simply clone the **multiphenicsx** public repository
```
git clone https://github.com/multiphenics/multiphenicsx.git
```
and install the package by typing
```
pip3 install .[tutorials]
```

#### 2.1. multiphenicsx docker image
If you want to try **multiphenicsx** out but do not have **FEniCSx** already installed, you can [pull our docker image from Docker Hub](https://hub.docker.com/r/multiphenics/multiphenicsx/). All required dependencies are already installed. **multiphenicsx** tutorials and tests are located at
```
/root/multiphenicsx
```

### 3. Tutorials
Several tutorials are provided in the [**tutorials** subfolder](https://github.com/multiphenics/multiphenicsx/tree/main/tutorials).
* **Tutorial 1**: block Poisson test case, to introduce facilities used in the library.
* **Tutorial 2**: Navier-Stokes problem using block matrices.
* **Tutorial 3**: weak imposition of Dirichlet boundary conditions by Lagrange multipliers using block matrices and discarding interior degrees of freedom.
* **Tutorial 4**: computation of the inf-sup constant for a Stokes problem assembled using block matrices.
* **Tutorial 5**: computation of the inf-sup constant for the problem presented in tutorial 3.
* **Tutorial 6**: several examples on optimal control problems, with different state equations (elliptic, Stokes, Navier-Stokes), control (distributed or boundary) and observation (distributed or boundary).
* **Tutorial 7**: understanding how restriction works in CG and DG spaces.
* **Tutorial 8**: how to use a restriction to perform local modifications to assembled tensors.
* **Tutorial 9**: applications of **multiphenicsx** to multiphysics problems. We are looking forward to receiving further multiphysics examples from our users!

### 4. Authors and contributors
**multiphenicsx** is currently developed and maintained at the [Catholic University of the Sacred Heart](https://www.unicatt.it/) by [Dr. Francesco Ballarin](https://www.francescoballarin.it) in collaboration with [Prof. Gianluigi Rozza](https://people.sissa.it/~grozza/)'s group at [SISSA mathLab](http://mathlab.sissa.it/). The financial support of the [AROMA-CFD ERC CoG project](https://people.sissa.it/~grozza/aroma-cfd/) is gratefully acknowledged. Please see the [AUTHORS file](https://github.com/multiphenics/multiphenicsx/blob/main/AUTHORS) for a list of contributors.

Contact us by [email](mailto:francesco.ballarin@unicatt.it) for further information or questions about **multiphenicsx**, or open an issue on [our issue tracker](https://github.com/multiphenics/multiphenicsx/issues). **multiphenicsx** is at an early development stage, so contributions improving either the code or the documentation are welcome, both as patches or [pull requests](https://github.com/multiphenics/multiphenicsx/pulls).

### 5. Related resources
* Block matrix support in [DOLFINx](https://github.com/FEniCS/dolfinx), either as MatNest or monolithic matrices. In **multiphenicsx** we also support possible restriction of the unknowns to subdomains and/or boundaries.
* Restriction support in [dolfiny](https://github.com/michalhabera/dolfiny) relies on assembling tensors on the whole domain, and the restricting them to subdomains and/or boundaries. In **multiphenicsx** we directly allocate the restricted tensors, so that no unnecessary memory allocations are carried out.
* Please contact us by [email](mailto:francesco.ballarin@unicatt.it) if you have other related resources.

### 6. How to cite
If you use **multiphenicsx** in your work, please cite the [multiphenics website](http://mathlab.sissa.it/multiphenics).

### 7. License
Like all core **FEniCS** components, **multiphenicsx** is freely available under the GNU LGPL, version 3.
