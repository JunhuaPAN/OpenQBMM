/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2015-2017 Alberto Passalacqua
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is derivative work of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::coalescenceEfficiencyKernels::Lehr

Description
    Coalesence efficiency based on film drainage and rupture times.

    \f[
        P_c = max(\frac{u_\mathrm{crit}}{u'}, 1)
        u_\mathrm{crit} = \sqrt{\frac{We_\mathrm{crit}\sigma}{\rho_f d_{eq} } }
        u' = \mathrm{max}(\sqrt{2}(\epsilon_f \sqrt{d_i d_j})^{1/3}, |\mathbf{U}_i - mathbf{U}_j|)
        d_{eq} = 2(\frac{1}{d_i} + \frac{1}{d_j})^{-1}
    \f]

    where

    \vartable
        \sigma      |  Surface tension
        \rho_f      |  Density of liquid phase [kg/m3]
        \epsilon_f  |  Turbulent dissipation rate of liquid phase [m2/s3]
        d_i         |  Diameter of bubble i [m]
        d_j         |  Diameter of bubble j [m]
        \mathbf{U}_i|  Velocity of bubble i [m/s]
        \mathbf{U}_j|  Velocity of bubble j [m/s]
    \endvartable

    References
    \verbatim
        "A transport equation for the interfacial area density applied to bubble
        columns"
        F. Lehr, D. Mewes
        Chemical Engineering Science
        Volume 56, Pages 1159-1166, 2001
    \endverbatim

SourceFiles
    Lehr.C

\*---------------------------------------------------------------------------*/

#ifndef Lehr_H
#define Lehr_H

#include "coalescenceEfficiencyKernel.H"
#include "twoPhaseSystem.H"
#include "phaseModel.H"
#include "PhaseCompressibleTurbulenceModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace coalescenceEfficiencyKernels
{

/*---------------------------------------------------------------------------*\
                    Class Lehr Declaration
\*---------------------------------------------------------------------------*/

class Lehr
:
    public coalescenceEfficiencyKernel
{
    // Private data

        //- Refrence to two phase system
        const twoPhaseSystem& fluid_;

        //- Switch if turbulence modleing is used
        Switch turbulence_;

        //- Surface tension
        dimensionedScalar sigma_;

        //- Critical velocity
        dimensionedScalar WeCrit_;

        //- Turbulent dissipation
        volScalarField epsilonf_;



public:

        //- Runtime type information
        TypeName("Lehr");


    // Constructors

        //- Construct from components
        Lehr
        (
            const dictionary& dict,
            const fvMesh& mesh
        );


    //- Destructor
    virtual ~Lehr();


    // Member Functions

        //- Update fields
        virtual void update();

        //- Aggregation kernel
        virtual tmp<volScalarField> Pc
        (
            const label nodei,
            const label nodej
        ) const;

        //- Aggregation kernel
        virtual scalar Pc
        (
            const label celli,
            const label nodei,
            const label nodej
        ) const;

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace coalescenceEfficiencyKernels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
