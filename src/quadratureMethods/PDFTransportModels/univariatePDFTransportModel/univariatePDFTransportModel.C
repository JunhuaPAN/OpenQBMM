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

\*---------------------------------------------------------------------------*/

#include "univariatePDFTransportModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::PDFTransportModels::univariatePDFTransportModel
::univariatePDFTransportModel
(
    const word& name,
    const dictionary& dict,
    const fvMesh& mesh,
    const surfaceScalarField& phi,
    const word& support
)
:
    PDFTransportModel(name, dict, mesh),
    name_(name),
    solveODESource_
    (
        dict.subDict("odeCoeffs").lookupOrDefault("solveODESource", true)
    ),
    ATol_(readScalar(dict.subDict("odeCoeffs").lookup("ATol"))),
    RTol_(readScalar(dict.subDict("odeCoeffs").lookup("RTol"))),
    fac_(readScalar(dict.subDict("odeCoeffs").lookup("fac"))),
    facMin_(readScalar(dict.subDict("odeCoeffs").lookup("facMin"))),
    facMax_(readScalar(dict.subDict("odeCoeffs").lookup("facMax"))),
    minLocalDt_(readScalar(dict.subDict("odeCoeffs").lookup("minLocalDt"))),
    quadrature_(name, mesh, support),
    momentAdvection_
    (
        univariateMomentAdvection::New
        (
            quadrature_.subDict("momentAdvection"),
            quadrature_,
            phi,
            support
        )
    )
{}

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::PDFTransportModels::univariatePDFTransportModel
::~univariatePDFTransportModel()
{}

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::PDFTransportModels::univariatePDFTransportModel
::explicitMomentSource()
{
    volUnivariateMomentFieldSet& moments(quadrature_.moments());
    label nMoments = quadrature_.nMoments();
    scalar globalDt = moments[0].mesh().time().deltaT().value();

    Info << "Solving source terms in realizable ODE solver." << endl;

    forAll(moments[0], celli)
    {
        // Storing old moments to recover from failed step

        scalarList oldMoments(nMoments, 0.0);

        forAll(oldMoments, mi)
        {
            oldMoments[mi] = moments[mi][celli];
        }

        //- Local time
        scalar localT = 0.0;

        // Initialize the local step
        scalar localDt = globalDt/100.0;

        // Initialize RK parameters
        scalarList k1(nMoments, 0.0);
        scalarList k2(nMoments, 0.0);
        scalarList k3(nMoments, 0.0);

        // Flag to indicate if the time step is complete
        bool timeComplete = false;

        // Check realizability of intermediate moment sets
        bool realizableUpdate1 = false;
        bool realizableUpdate2 = false;
        bool realizableUpdate3 = false;

        scalarList momentsSecondStep(nMoments, 0.0);

        while (!timeComplete)
        {
            do
            {
                // First intermediate update
                forAll(oldMoments, mi)
                {
                    k1[mi] = localDt*cellMomentSource(mi, celli);
                    moments[mi][celli] = oldMoments[mi] + k1[mi];
                }

                realizableUpdate1 =
                        quadrature_.updateLocalQuadrature(celli, false);

                quadrature_.updateLocalMoments(celli);

                // Second moment update
                forAll(oldMoments, mi)
                {
                    k2[mi] = localDt*cellMomentSource(mi, celli);
                    moments[mi][celli] = oldMoments[mi] + (k1[mi] + k2[mi])/4.0;

                    momentsSecondStep[mi] = moments[mi][celli];
                }

                realizableUpdate2 =
                        quadrature_.updateLocalQuadrature(celli, false);

                quadrature_.updateLocalMoments(celli);

                // Third moment update
                forAll(oldMoments, mi)
                {
                    k3[mi] = localDt*cellMomentSource(mi, celli);
                    moments[mi][celli] =
                        oldMoments[mi] + (k1[mi] + k2[mi] + 4.0*k3[mi])/6.0;
                }

                realizableUpdate3 =
                        quadrature_.updateLocalQuadrature(celli, false);

                quadrature_.updateLocalMoments(celli);

                if
                (
                    !realizableUpdate1
                 || !realizableUpdate2
                 || !realizableUpdate3
                )
                {
                    Info << "Not realizable" << endl;

                    forAll(oldMoments, mi)
                    {
                        moments[mi][celli] = oldMoments[mi];
                    }

                    // Updating local quadrature with old moments
                    quadrature_.updateLocalQuadrature(celli);

                    localDt /= 2.0;

                    if (localDt < minLocalDt_)
                    {
                        FatalErrorInFunction
                            << "Reached minimum local step in realizable ODE"
                            << nl
                            << "    solver. Cannot ensure realizability." << nl
                            << abort(FatalError);
                    }
                }
            }
            while
            (
                !realizableUpdate1
             || !realizableUpdate2
             || !realizableUpdate3
            );

            scalar error = 0.0;

            for (label mi = 0; mi < nMoments; mi++)
            {
                scalar scalei =
                        ATol_
                    + max
                        (
                            mag(momentsSecondStep[mi]), mag(oldMoments[mi])
                        )*RTol_;

                error +=
                        sqr
                        (
                            (momentsSecondStep[mi] - moments[mi][celli])/scalei
                        );
            }

            error = sqrt(error/nMoments);

            if (error < 1)
            {
                localDt *= min(facMax_, max(facMin_, fac_/pow(error, 1.0/3.0)));

                scalar maxLocalDt = max(globalDt - localT, 0.0);
                localDt = min(maxLocalDt, localDt);

                forAll(oldMoments, mi)
                {
                    oldMoments[mi] = moments[mi][celli];
                }

                if (localDt == 0.0)
                {
                    timeComplete = true;
                    localT = 0.0;
                    break;
                }

                localT += localDt;
            }
            else
            {
                localDt *= min(1.0, max(facMin_, fac_/pow(error, 1.0/3.0)));

                forAll(oldMoments, mi)
                {
                    moments[mi][celli] = oldMoments[mi];
                }
            }
        }
    }
}

void Foam::PDFTransportModels::univariatePDFTransportModel::solve()
{
    momentAdvection_().update();

    // List of moment transport equations
    PtrList<fvScalarMatrix> momentEqns(quadrature_.nMoments());

    // Solve moment transport equations
    forAll(quadrature_.moments(), momenti)
    {
        volUnivariateMoment& m = quadrature_.moments()[momenti];

        momentEqns.set
        (
            momenti,
            new fvScalarMatrix
            (
                fvm::ddt(m)
              + momentAdvection_().divMoments()[momenti]
              - momentDiffusion(m)
              ==
                implicitMomentSource(m)
            )
        );
    }

    if (solveODESource_)
    {
        explicitMomentSource();
    }

    forAll (momentEqns, mEqni)
    {
        volUnivariateMoment& m = quadrature_.moments()[mEqni];

        if (solveODESource_)
        {
            momentEqns[mEqni] -= fvc::ddt(m);
        }

        momentEqns[mEqni].relax();
        momentEqns[mEqni].solve();
    }

    quadrature_.updateQuadrature();
    //quadrature_.updateMoments();
}


// ************************************************************************* //
