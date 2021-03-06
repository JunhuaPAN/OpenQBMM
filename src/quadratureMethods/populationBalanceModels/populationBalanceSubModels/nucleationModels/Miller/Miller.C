/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 Alberto Passalacqua
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

\*---------------------------------------------------------------------------*/

#include "Miller.H"
#include "addToRunTimeSelectionTable.H"
#include "fundamentalConstants.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace populationBalanceSubModels
{
namespace nucleationModels
{
    defineTypeNameAndDebug(Miller, 0);

    addToRunTimeSelectionTable
    (
        nucleationModel,
        Miller,
        dictionary
    );
}
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::populationBalanceSubModels::nucleationModels::Miller::Miller
(
    const dictionary& dict,
    const fvMesh& mesh
)
:
    nucleationModel(dict, mesh),
    MCarbon_(dict.lookup("MCarbon")),
    nCarbonDimer_(dict.lookup("nCarbonDimer")),
    nCarbonPAM_(dict.lookup("nCarbonPAM")),
    rhoSoot_(dict.lookup("rhoSoot")),
    pamConcentration_
    (
        IOobject
        (
            "YPAM",
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    ),
    flThermo_(mesh_.lookupObject<fluidThermo>(basicThermo::dictName)),
    T_(flThermo_.T())
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::populationBalanceSubModels::nucleationModels::Miller::~Miller()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar
Foam::populationBalanceSubModels::nucleationModels::Miller::nucleationSource
(
    const label& momentOrder,
    const label celli,
    const label environment
) const
{
    scalar NA = Foam::constant::physicoChemical::NA.value();
    scalar MCarbon = MCarbon_.value();

    scalar abscissaNucleation =
        2.0*MCarbon*nCarbonDimer_.value()/(rhoSoot_.value()*NA);

    return 4.4*sqrt(Foam::constant::mathematical::pi
        *Foam::constant::physicoChemical::k.value()*T_[celli]*NA
        /nCarbonPAM_.value()*MCarbon)*pow(6.0*nCarbonPAM_.value()*MCarbon
        /(Foam::constant::mathematical::pi*rhoSoot_.value()
        *NA), 2.0/3.0)*NA*sqr(pamConcentration_[celli])
        *pow(abscissaNucleation, momentOrder);
}

// ************************************************************************* //
