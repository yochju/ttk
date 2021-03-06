/// \ingroup baseCode
/// \class ttk::PersistenceCurve
/// \author Guillaume Favelier <guillaume.favelier@lip6.fr>
/// \author Julien Tierny <julien.tierny@lip6.fr>
/// \date September 2016.
///
/// \brief TTK processing package for the computation of persistence curves.
///
/// This package computes the list of extremum-saddle pairs and computes the 
/// number of pairs as a function of persistence (i.e. the number of pairs 
/// whose persistence is higher than a threshold).
///
/// These curves provide useful visual clues in order to fine-tune persistence
/// simplification thresholds.
///
/// \sa vtkPersistenceCurve.cpp %for a usage example.

#ifndef _PERSISTENCECURVE_H
#define _PERSISTENCECURVE_H

// base code includes
#include<Wrapper.h>
#include<ContourForests.h>
#include<DiscreteGradient.h>
#include<Triangulation.h>

namespace ttk{

  class PersistenceCurve : public Debug{

    public:

      PersistenceCurve();
      ~PersistenceCurve();

      inline int setComputeSaddleConnectors(bool state){
        ComputeSaddleConnectors=state;
        return 0;
      }

      template <typename scalarType>
        int computePersistencePlot(const vector<tuple<idVertex, idVertex, scalarType>>& pairs,
            vector<pair<scalarType, idVertex>> &plot) const;

      template <class scalarType>
        int execute() const;

      inline int setupTriangulation(Triangulation* data){
        triangulation_ = data;
        if(triangulation_){
          ContourForests contourTree;
          contourTree.setupTriangulation(triangulation_);

          triangulation_->preprocessBoundaryVertices();
        }
        return 0;
      }

      inline int setInputScalars(void* data){
        inputScalars_ = data;
        return 0;
      }

      inline int setInputOffsets(void* data){
        inputOffsets_ = data;
        return 0;
      }

      inline int setOutputJTPlot(void* data){
        JTPlot_=data;
        return 0;
      }

      inline int setOutputSTPlot(void* data){
        STPlot_=data;
        return 0;
      }

      inline int setOutputMSCPlot(void* data){
        MSCPlot_=data;
        return 0;
      }

      inline int setOutputCTPlot(void* data){
        CTPlot_=data;
        return 0;
      }

    protected:

      bool ComputeSaddleConnectors;

      Triangulation* triangulation_;
      void* inputScalars_;
      void* inputOffsets_;
      void* JTPlot_;
      void* MSCPlot_;
      void* STPlot_;
      void* CTPlot_;
  };
}

template <typename scalarType>
int PersistenceCurve::computePersistencePlot(const vector<tuple<idVertex, idVertex, scalarType>>& pairs,
    vector<pair<scalarType, idVertex>> &plot) const{

  idVertex nbElmnt = pairs.size();
  plot.resize(nbElmnt);

  // build curve
  const scalarType epsilon=static_cast<scalarType>(pow(10, -REAL_SIGNIFICANT_DIGITS));
  for (idVertex i = 0; i < nbElmnt; ++i) {
    plot[i].first  = std::max(get<2>(pairs[i]), epsilon);
    plot[i].second = pairs.size() - i;
  }

  return 0;
}

template <typename scalarType>
int PersistenceCurve::execute() const{
  // get data
  vector<pair<scalarType, idVertex>>& JTPlot=*static_cast<vector<pair<scalarType, idVertex>>*>(JTPlot_);
  vector<pair<scalarType, idVertex>>& STPlot=*static_cast<vector<pair<scalarType, idVertex>>*>(STPlot_);
  vector<pair<scalarType, idVertex>>& MSCPlot=*static_cast<vector<pair<scalarType, idVertex>>*>(MSCPlot_);
  vector<pair<scalarType, idVertex>>& CTPlot=*static_cast<vector<pair<scalarType, idVertex>>*>(CTPlot_);
  int* offsets=static_cast<int*>(inputOffsets_);
  scalarType* scalars=static_cast<scalarType*>(inputScalars_);

  const idVertex numberOfVertices=triangulation_->getNumberOfVertices();
  // convert offsets into a valid format for contour forests
  vector<idVertex> voffsets(numberOfVertices);
  std::copy(offsets,offsets+numberOfVertices,voffsets.begin());

  // get contour tree
  ContourForests contourTree;
  contourTree.setupTriangulation(triangulation_, false);
  contourTree.setVertexScalars(inputScalars_);
  contourTree.setTreeType(TreeType::Join);
  contourTree.setVertexSoSoffsets(voffsets);
  contourTree.setLessPartition(true);
  // at the moment, only one thread is supported
  //contourTree->setThreadNumber(threadNumber_);
  contourTree.setThreadNumber(1);
  contourTree.build<scalarType>();

  // get persistence pairs
  vector<tuple<idVertex, idVertex, scalarType>> JTPairs;
  vector<tuple<idVertex, idVertex, scalarType>> STPairs;
#ifdef withOpenMP
#pragma omp parallel sections
#endif
  {
#ifdef withOpenMP
#pragma omp section
#endif
    contourTree.getJoinTree()->computePersistencePairs<scalarType>(JTPairs);
#ifdef withOpenMP
#pragma omp section
#endif
    contourTree.getSplitTree()->computePersistencePairs<scalarType>(STPairs);
  }

  // merge pairs
  vector<tuple<idVertex, idVertex, scalarType>> CTPairs(JTPairs.size()+STPairs.size());
  std::copy(JTPairs.begin(), JTPairs.end(), CTPairs.begin());
  std::copy(STPairs.begin(), STPairs.end(), CTPairs.begin()+JTPairs.size());
  {
    auto cmp=[](const tuple<idVertex,idVertex,scalarType>& a,
        const tuple<idVertex,idVertex,scalarType>& b){
      return get<2>(a) < get<2>(b);
    };
    std::sort(CTPairs.begin(), CTPairs.end(), cmp);
  }

  // get the saddle-saddle pairs
  vector<tuple<int,int,scalarType>> pl_saddleSaddlePairs;
  const int dimensionality=triangulation_->getDimensionality();
  if(dimensionality==3 and ComputeSaddleConnectors){
    // get original list of critical points
    vector<pair<int,char>> pl_criticalPoints;
    {
      const int* const offsets=static_cast<int*>(inputOffsets_);
      vector<int> sosOffsets(numberOfVertices);
      for(int i=0; i<numberOfVertices; ++i)
        sosOffsets[i]=offsets[i];

      ScalarFieldCriticalPoints<scalarType> scp;

      scp.setDebugLevel(debugLevel_);
      scp.setThreadNumber(threadNumber_);
      scp.setDomainDimension(dimensionality);
      scp.setScalarValues(inputScalars_);
      scp.setVertexNumber(numberOfVertices);
      scp.setSosOffsets(&sosOffsets);
      scp.setupTriangulation(triangulation_);
      scp.setOutput(&pl_criticalPoints);

      scp.execute();
    }

    // build rejecting list
    vector<char> isRejected(numberOfVertices, false);
    for(const auto& i : CTPairs){
      const int v0=get<0>(i);
      const int v1=get<1>(i);
      isRejected[v0]=true;
      isRejected[v1]=true;
    }

    // filter the critical points according to the filtering list and boundary condition
    vector<char> isSaddle1(numberOfVertices, false);
    vector<char> isSaddle2(numberOfVertices, false);
    vector<pair<int,char>> pl_filteredCriticalPoints;
    for(const auto& i : pl_criticalPoints){
      const int vertexId=i.first;
      const char type=i.second;
      if(!isRejected[vertexId]){
        pl_filteredCriticalPoints.push_back(i);

        switch(type){
          case 1:
          isSaddle1[vertexId]=true;
          break;

          case 2:
          isSaddle2[vertexId]=true;
          break;
        }
      }
    }

    vector<tuple<Cell,Cell>> dmt_pairs;
    {
      // simplify to be PL-conformant
      DiscreteGradient discreteGradient;
      discreteGradient.setDebugLevel(debugLevel_);
      discreteGradient.setThreadNumber(threadNumber_);
      discreteGradient.setupTriangulation(triangulation_);
      discreteGradient.setIterationThreshold(-1);
      discreteGradient.setReverseSaddleMaximumConnection(true);
      discreteGradient.setReverseSaddleSaddleConnection(true);
      discreteGradient.setAllowReversingWithNonRemovable(false);
      discreteGradient.setCollectPersistencePairs(false);
      discreteGradient.setInputScalarField(inputScalars_);
      discreteGradient.setInputOffsets(inputOffsets_);
      discreteGradient.buildGradient<scalarType>();
      discreteGradient.reverseGradient<scalarType>(pl_criticalPoints);

      // collect saddle-saddle connections
      discreteGradient.setReverseSaddleMaximumConnection(false);
      discreteGradient.setCollectPersistencePairs(true);
      discreteGradient.setOutputPersistencePairs(&dmt_pairs);
      discreteGradient.reverseGradient<scalarType>(pl_filteredCriticalPoints);
    }

    // transform DMT pairs into PL pairs
    for(const auto& i : dmt_pairs){
      const Cell& saddle1=get<0>(i);
      const Cell& saddle2=get<1>(i);

      int v0=-1;
      for(int j=0; j<2; ++j){
        int vertexId;
        triangulation_->getEdgeVertex(saddle1.id_, j, vertexId);

        if(isSaddle1[vertexId]){
          v0=vertexId;
          break;
        }
      }
      if(v0==-1){
        scalarType scalar{};
        for(int j=0; j<2; ++j){
          int vertexId;
          triangulation_->getEdgeVertex(saddle1.id_,j,vertexId);
          const scalarType vertexScalar=scalars[vertexId];

          if(!j or scalar>vertexScalar){
            v0=vertexId;
            scalar=vertexScalar;
          }
        }
      }

      int v1=-1;
      for(int j=0; j<3; ++j){
        int vertexId;
        triangulation_->getTriangleVertex(saddle2.id_, j, vertexId);

        if(isSaddle2[vertexId]){
          v1=vertexId;
          break;
        }
      }
      if(v1==-1){
        scalarType scalar{};
        for(int j=0; j<3; ++j){
          int vertexId;
          triangulation_->getTriangleVertex(saddle2.id_,j,vertexId);
          const scalarType vertexScalar=scalars[vertexId];

          if(!j or scalar<vertexScalar){
            v1=vertexId;
            scalar=vertexScalar;
          }
        }
      }

      const scalarType persistence=scalars[v1]-scalars[v0];

      if(v0!=-1 and v1!=-1 and persistence>=0)
        pl_saddleSaddlePairs.push_back(make_tuple(v0,v1,persistence));
    }
  }

  // sort the saddle-saddle pairs by persistence value and compute curve
  if(dimensionality==3 and ComputeSaddleConnectors){
    {
      auto cmp=[](const tuple<idVertex,idVertex,scalarType>& a,
          const tuple<idVertex,idVertex,scalarType>& b){
        return get<2>(a) < get<2>(b);
      };
      std::sort(pl_saddleSaddlePairs.begin(), pl_saddleSaddlePairs.end(), cmp);
    }

    computePersistencePlot<scalarType>(pl_saddleSaddlePairs, MSCPlot);
  }

  // get persistence curves
#ifdef withOpenMP
#pragma omp parallel sections
#endif
  {
#ifdef withOpenMP
#pragma omp section
#endif
    if(JTPlot_)
      computePersistencePlot<scalarType>(JTPairs, JTPlot);
#ifdef withOpenMP
#pragma omp section
#endif
    if(STPlot_)
      computePersistencePlot<scalarType>(STPairs, STPlot);
#ifdef withOpenMP
#pragma omp section
#endif
    if(CTPlot_)
      computePersistencePlot<scalarType>(CTPairs, CTPlot);
  }

  return 0;
}

#endif // PERSISTENCECURVE_H
