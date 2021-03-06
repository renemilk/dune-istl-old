#ifndef DUNE_AMG_GRAPHCREATOR_HH
#define DUNE_AMG_GRAPHCREATOR_HH

#include"graph.hh"
#include"dependency.hh"
#include"pinfo.hh"
#include<dune/istl/operators.hh>
#include<dune/istl/bcrsmatrix.hh>
#include<dune/common/tuples.hh>

namespace Dune
{
  namespace Amg
  {
    template<class M, int cat=M::category>
    struct PropertiesGraphCreator
    {
    };
    
    template<class M>
    struct PropertiesGraphCreator<M,SolverCategory::sequential>
    {
      typedef typename M::matrix_type Matrix;
      
      typedef Dune::Amg::MatrixGraph<const Matrix> MatrixGraph;
      
      typedef Dune::Amg::PropertiesGraph<MatrixGraph,
			      VertexProperties,
			      EdgeProperties,
			      IdentityMap,
			      IdentityMap> PropertiesGraph;
      
      typedef Dune::tuple<MatrixGraph*,PropertiesGraph*> GraphTuple;
      
      template<class OF, class T>
      static GraphTuple create(const M& matrix, T& excluded,
			       const SequentialInformation& pinfo,
			       const OF&)
      {
	MatrixGraph* mg = new MatrixGraph(matrix.getmat());
	PropertiesGraph* pg = new PropertiesGraph(*mg, IdentityMap(), IdentityMap());
	return GraphTuple(mg,pg);
      }
      
      static void free(GraphTuple& graphs)
      {
	delete get<1>(graphs);
      }
      
    };

    template<class M>
    struct PropertiesGraphCreator<M,SolverCategory::overlapping>
    {
      typedef typename M::matrix_type Matrix;
      typedef Dune::Amg::MatrixGraph<const Matrix> MatrixGraph;
      typedef Dune::Amg::SubGraph<MatrixGraph,
		       std::vector<bool> > SubGraph;
      typedef Dune::Amg::PropertiesGraph<SubGraph,
			      VertexProperties,
			      EdgeProperties,
			      IdentityMap,
			      typename SubGraph::EdgeIndexMap>
      PropertiesGraph;
    
      typedef Dune::tuple<MatrixGraph*,PropertiesGraph*,SubGraph*> GraphTuple;
      
      template<class OF, class T, class PI>
      static GraphTuple create(const M& matrix, T& excluded, 
			       PI& pinfo, const OF& of)
      {
	typedef OF OverlapFlags;
	MatrixGraph* mg = new MatrixGraph(matrix.getmat());
	typedef typename PI::ParallelIndexSet ParallelIndexSet;
	typedef typename ParallelIndexSet::const_iterator IndexIterator;
	IndexIterator iend = pinfo.indexSet().end();
	
	for(IndexIterator index = pinfo.indexSet().begin(); index != iend; ++index)
	  excluded[index->local()] = of.contains(index->local().attribute());
	
	SubGraph* sg= new SubGraph(*mg, excluded);
	PropertiesGraph* pg = new PropertiesGraph(*sg, IdentityMap(), sg->getEdgeIndexMap());
	return GraphTuple(mg,pg,sg);
      }

      static void free(GraphTuple& graphs)
      {
	delete get<2>(graphs);
	delete get<1>(graphs);
      }
    };

    template<class M>
    struct PropertiesGraphCreator<M,SolverCategory::nonoverlapping>
    {
      typedef typename M::matrix_type Matrix;
      typedef Dune::Amg::MatrixGraph<const Matrix> MatrixGraph;
      typedef Dune::Amg::SubGraph<MatrixGraph,
		       std::vector<bool> > SubGraph;
      typedef Dune::Amg::PropertiesGraph<SubGraph,
			      VertexProperties,
			      EdgeProperties,
			      IdentityMap,
			      typename SubGraph::EdgeIndexMap>
      PropertiesGraph;
    
      typedef Dune::tuple<MatrixGraph*,PropertiesGraph*,SubGraph*> GraphTuple;
      
      template<class OF, class T, class PI>
      static GraphTuple create(const M& matrix, T& excluded, 
			       PI& pinfo, const OF& of)
      {
	typedef OF OverlapFlags;
	MatrixGraph* mg = new MatrixGraph(matrix.getmat());
	typedef typename PI::ParallelIndexSet ParallelIndexSet;
	typedef typename ParallelIndexSet::const_iterator IndexIterator;
	IndexIterator iend = pinfo.indexSet().end();
	
	for(IndexIterator index = pinfo.indexSet().begin(); index != iend; ++index)
	  excluded[index->local()] = of.contains(index->local().attribute());
	
	SubGraph* sg= new SubGraph(*mg, excluded);
	PropertiesGraph* pg = new PropertiesGraph(*sg, IdentityMap(), sg->getEdgeIndexMap());
	return GraphTuple(mg,pg,sg);
      }

      static void free(GraphTuple& graphs)
      {
	delete get<2>(graphs);
	delete get<1>(graphs);
      }
    };
    
  }//namespace Amg
} // namespace Dune
#endif
