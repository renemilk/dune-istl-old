// $Id$
#ifndef DUNE_COMMUNICATOR
#define DUNE_COMMUNICATOR

#include"remoteindices.hh"
#include"interface.hh"
#include<dune/common/exceptions.hh>
#include<dune/common/typetraits.hh>
namespace Dune
{
  /** @defgroup ISTL_Comm ISTL Communication
   * @ingroup ISTL
   * @brief Provides classes for syncing distributed indexed 
   * data structures.
   */
  /** @addtogroup ISTL_Comm
   *
   * @{
   */
  /**
   * @file
   * @brief Provides utility classes for syncing distributed data via 
   * MPI communication.
   * @author Markus Blatt
   */    

  /**
   * @brief Flag for marking indexed data structures where data at 
   * each index is of the same size.
   * @see VariableSize 
   */
  struct SizeOne
  {};
  
  /**
   * @brief Flag for marking indexed data structures where the data at each index may
   * be a variable multiple of another type.
   * @see SizeOne
   */
  struct VariableSize
  {};
  

  /**
   * @brief Default policy used for communicating an indexed type.
   *
   * This
   */
  template<class V>
  struct CommPolicy
  {
    /** 
     * @brief The type the policy is for.
     *
     * It has to provide the mode
     * <pre> Type::IndexedType operator[](int i);</pre>
     * for
     * the access of the value at index i and a typedef IndexedType. 
     * It is assumed 
     * that only one entry is at each index (as in scalar
     * vector.
    */
    typedef V Type;
    
    /**
     * @brief The type we get at each index with operator[].
     *
     * The default is the value_type typedef of the container.
     */
    typedef typename V::value_type IndexedType;
    
    /**
     * @brief Whether the indexed type has variable size or there
     * is always one value at each index.
     */
    typedef SizeOne IndexedTypeFlag;

    /**
     * @brief Get the address of entry at an index.
     *
     * The default implementation uses operator[] to
     * get the address.
     * @param v An existing representation of the type that has more elements than index.
     * @param index The index of the entry.
     */
    static const void* getAddress(const V& v, int index);
    
    /**
     * @brief Get the number of primitve elements at that index.
     *
     * The default always returns 1.
     */
    static int getSize(const V&, int index);
  };

  template<class K, int n> class FieldVector;
  
  template<class B, class A> class VariableBlockVector;
  
  template<class K, class A, int n>
  struct CommPolicy<VariableBlockVector<FieldVector<K, n>, A> >
  {
    typedef VariableBlockVector<FieldVector<K, n>, A> Type;
    
    typedef typename Type::B IndexedType;
    
    typedef VariableSize IndexedTypeFlag;

    static const void* getAddress(const Type& v, int i);
    
    static int getSize(const Type& v, int i);
  };
    
  /**
   * @brief Error thrown if there was a problem with the communication.
   */
  class CommunicationError : public IOError
  {};

  /**
   * @brief GatherScatter default implementation that just copies data.
   */
  template<class T>
  struct CopyGatherScatter
  {
    typedef typename CommPolicy<T>::IndexedType IndexedType;
    
    const IndexedType& gather(const T& vec, int i);
    
    void scatter(T& vec, const IndexedType& v, int i);
    
  };

  /**
   * @brief An utility class for communicating distributed data structures via MPI datatypes.
   *
   * This communicator creates special MPI datatypes that address the non contiguous elements
   * to be send and received. The idea was to prevent the copying to an additional buffer and
   * the mpi implementation decide whether to allocate buffers or use buffers offered by the 
   * interconnection network.
   *
   * Unfortunately the implementation of MPI datatypes seems to be poor. Therefore for most MPI 
   * implementations using a BufferedCommunicator will be more efficient.
   */
  template<typename T>
  class DatatypeCommunicator : public InterfaceBuilder<T>
  {
  public:    

    /**
     * @brief Type of the index set.
     */
    typedef T ParallelIndexSet;

    /**
     * @brief Type of the underlying remote indices class.
     */
    typedef RemoteIndices<ParallelIndexSet> RemoteIndices;

    /**
     * @brief The type of the global index.
     */
    typedef typename RemoteIndices::GlobalIndex GlobalIndex;
    
    /**
     * @brief The type of the attribute.
     */
    typedef typename RemoteIndices::Attribute Attribute;

    /**
     * @brief The type of the local index.
     */
    typedef typename RemoteIndices::LocalIndex LocalIndex;
    
    /**
     * @brief Creates a new DatatypeCommunicator.
     */
    DatatypeCommunicator();
    
    /**
     * @brief Destructor.
     */
    ~DatatypeCommunicator();

    /**
     * @brief Builds the interface between the index sets.
     *
     * Has to be called before the actual communication by forward or backward
     * can be called. Nonpublic indices will be ignored!
     *
     * 
     * The types T1 and T2 are classes representing a set of 
     * enumeration values of type DatatypeCommunicator::Attribute. 
     * They have to provide
     * a (static) method
     * <pre>
     * bool contains(Attribute flag) const;
     * </pre>
     * for checking whether the set contains a specfic flag. 
     * This functionality is for example provided the classes
     * EnumItem, EnumRange and Combine.
     *
     * @param remoteIndices The indices present on remote processes.
     * @param sourceFlags The set of attributes which mark indices we send to other
     *               processes.
     * @param sendData The indexed data structure whose data will be send.
     * @param destFlags  The set of attributes which mark the indices we might
     *               receive values from.
     * @param receiveData The indexed data structure for which we receive data.
     */
    template<class T1, class T2, class V>
    void build(const RemoteIndices& remoteIndices, const T1& sourceFlags, V& sendData, const T2& destFlags, V& receiveData);
    
    /**
     * @brief Sends the primitive values from the source to the destination.
     */
    void forward();
    
    /**
     * @brief Sends the primitive values from the destination to the source.
     */
    void backward();
   
    /**
     * @brief Deallocates the MPI requests and data types.
     */
    void free();        
  private:
    enum { 
      /**
       * @brief Tag for the MPI communication.
       */
      commTag_ = 234
    };
    
    /**
     * @brief The indices also known at other processes.
     */
    const RemoteIndices* remoteIndices_;
    
    typedef std::map<int,std::pair<MPI_Datatype,MPI_Datatype> > 
    MessageTypeMap;
    
    /**
     * @brief The datatypes built according to the communication interface.
     */
    MessageTypeMap messageTypes;
    
    /**
     * @brief The pointer to the data whose entries we communicate.
     */
    void* data_;

    MPI_Request* requests_[2];

    /**
     * @brief True if the request and data types were created.
     */
    bool created_;
    
    /**
     * @brief Creates the MPI_Requests for the forward communication.
     */
    template<class V, bool FORWARD>
    void createRequests(V& sendData, V& receiveData);
    
    /**
     * @brief Creates the data types needed for the unbuffered receive.
     */
    template<class T1, class T2, class V, bool send>
    void createDataTypes(const T1& source, const T2& destination, V& data);

    /**
     * @brief Initiates the sending and receive.
     */
    void sendRecv(MPI_Request* req);
    
    /**
     * @brief Information used for setting up the MPI Datatypes.
     */
    struct IndexedTypeInformation
    {
      /**
       * @brief Allocate space for setting up the MPI datatype.
       *
       * @param i The number of values the datatype will have.
       */
      void build(int i)
      {
	length = new int[i];
	displ  = new MPI_Aint[i];
	size = i;
      }
      
      /**
       * @brief Free the allocated space.
       */
      void free()
      {
	delete[] length;
	delete[] displ;
      }
      /**  @brief The number of values at each index. */
      int* length;
      /** @brief The displacement at each index. */
      MPI_Aint* displ;
      /** 
       * @brief The number of elements we send.
       * In case of variable sizes this will differ from
       * size.
       */
      int elements;
      /**
       * @param The number of indices in the data type.
       */
      int size;
    };
    
    /**
     * @brief Functor for the InterfaceBuilder.
     *
     * It will record the information needed to build the MPI_Datatypes.
     */
    template<class V>
    struct MPIDatatypeInformation
    {
      /**
       * @brief Constructor.
       * @param data The data we construct an MPI data type for.
       */
      MPIDatatypeInformation(const V& data): data_(data)
      {}
      
      /**
       * @brief Reserver space for the information about the datatype.
       * @param proc The rank of the process this information is for.
       * @param size The number of indices the datatype will contain.
       */
      void reserve(int proc, int size)
      {
	information_[proc].build(size);
      }
      /**
       * @brief Add a new index to the datatype.
       * @param proc The rank of the process this index is send to
       * or received from.
       * @param local The index to add.
       */
      void add(int proc, int local)
      {
	IndexedTypeInformation& info=information_[proc];
	assert(info.elements<info.size);
	MPI_Address( const_cast<void*>(CommPolicy<V>::getAddress(data_, local)),
		    info.displ+info.elements);
	info.length[info.elements]=CommPolicy<V>::getSize(data_, local);
	info.elements++;
      }
      
      /** 
       * @brief The information about the datatypes to send to or
       * receive from each process.
       */
      std::map<int,IndexedTypeInformation> information_;
      /**
       * @brief A representative of the indexed data we send.
       */
      const V& data_;
      
    };
    
  };

  /**
   * @brief A communicator that uses buffers to gather and scatter
   * the data to be send or received.
   *
   * Before the data is sent it it copied to a consecutive buffer and
   * then that buffer is sent.
   * The data is received in another buffer and then copied to the actual
   * position.
   */
  template<typename T>
  class BufferedCommunicator
  {
  public:
    /**
     * @brief The type of the interface.
     */
    typedef Interface<T> Interface;
    
    /**
     * @brief The type of the global index.
     */
    typedef typename Interface::GlobalIndex GlobalIndex;
    
    /**
     * @brief The type of the attributes.
     */
    typedef typename Interface::Attribute Attribute;
    
    /**
     * @brief Constructor.
     */
    BufferedCommunicator();
    
    /**
     * @brief Build the buffers and information for the communication process.
     *
     * 
     * @param interface The interface that defines what indices are to be communicated.
     */
    template<class Data>
    typename EnableIf<is_same<SizeOne,typename CommPolicy<Data>::IndexedTypeFlag>::value, void>::Type
    build(const Interface& interface);

    /**
     * @brief Build the buffers and information for the communication process.
     *
     * @param source The source in a forward send. The values will be copied from here to the send buffers.
     * @param target The target in a forward send. The received values will be copied to here.
     * @param interface The interface that defines what indices are to be communicated.
     */
    template<class Data>
    void build(const Data& source, const Data& target, const Interface& interface);
    
    /**
     * @brief Send from source to target.
     *
     * The template parameter GatherScatter has to have a static method
     * <pre>
     * // Gather the data at index index of data
     * static const typename CommPolicy<Data>::IndexedType>& gather(Data& data, int index);
     *
     * // Scatter the value at a index of data
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is SizeOne
     * and
     * 
     * <pre>
     * static const typename CommPolicy<Data>::IndexedType> gather(Data& data, int index, int subindex);
     *
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index, int subindex);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is VariableSize. Here subindex is the
     * subindex of the block at index.
     * @warning The source and target data have to have the same layout as the ones given
     * to the build function in case of variable size values at the indices.
     * @param source The values will be copied from here to the send buffers. 
     * @param dest The received values will be copied to here.
     */
    template<class GatherScatter, class Data>
    void forward(const Data& source, Data& dest);
    
    /**
     * @brief Communicate in the reverse direction, i.e. send from target to source.
     *
     * The template parameter GatherScatter has to have a static method
     * <pre>
     * // Gather the data at index index of data
     * static const typename CommPolicy<Data>::IndexedType>& gather(Data& data, int index);
     *
     * // Scatter the value at a index of data
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is SizeOne
     * and
     * 
     * <pre>
     * static onst typename CommPolicy<Data>::IndexedType> gather(Data& data, int index, int subindex);
     *
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index, int subindex);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is VariableSize. Here subindex is the
     * subindex of the block at index.
     * @warning The source and target data have to have the same layout as the ones given
     * to the build function in case of variable size values at the indices.
     * @param dest The values will be copied from here to the send buffers. 
     * @param source The received values will be copied to here.
     */
    template<class GatherScatter, class Data>
    void backward(Data& source, const Data& dest);

    /**
     * @brief Forward send where target and source are the same.
     *
     * The template parameter GatherScatter has to have a static method
     * <pre>
     * // Gather the data at index index of data
     * static const typename CommPolicy<Data>::IndexedType>& gather(Data& data, int index);
     *
     * // Scatter the value at a index of data
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is SizeOne
     * and
     * 
     * <pre>
     * static onst typename CommPolicy<Data>::IndexedType> gather(Data& data, int index, int subindex);
     *
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index, int subindex);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is VariableSize. Here subindex is the
     * subindex of the block at index.
     * @param data Source and target of the communication.
     */
    template<class GatherScatter, class Data>
    void forward(Data& data);
    
    /**
     * @brief Backward send where target and source are the same.
     *
     * The template parameter GatherScatter has to have a static method
     * <pre>
     * // Gather the data at index index of data
     * static const typename CommPolicy<Data>::IndexedType>& gather(Data& data, int index);
     *
     * // Scatter the value at a index of data
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is SizeOne
     * and
     * 
     * <pre>
     * static onst typename CommPolicy<Data>::IndexedType> gather(Data& data, int index, int subindex);
     *
     * static void scatter(Data& data, typename CommPolicy<Data>::IndexedType> value,
     *                     int index, int subindex);
     * </pre>
     * in the case where CommPolicy<Data>::IndexedTypeFlag is VariableSize. Here subindex is the
     * subindex of the block at index.
     * @param data Source and target of the communication.
     */
    template<class GatherScatter, class Data>
    void backward(Data& data);
    
    /**
     * @brief Free the allocated memory (i.e. buffers and message information.
     */
    void free();
    
    /**
     * @brief Destructor.
     */
    ~BufferedCommunicator();
    
  private:

    /**
     * @brief Functors for message size caculation
     */
    template<class Data, typename IndexedTypeFlag>
    struct MessageSizeCalculator
    {};
    
    /**
     * @brief Functor for message size caculation for datatypes
     * where at each index is only one value.
     */
    template<class Data>
    struct MessageSizeCalculator<Data,SizeOne>
    {
      /**
       * @brief Calculate the number of values in message
       * @param info The information about the interface corresponding
       * to the message.
       * @return The number of values in th message.
       */
      inline int operator()(const InterfaceInformation& info) const;
      /**
       * @brief Calculate the number of values in message
       *
       * @param info The information about the interface corresponding
       * to the message.
       * @param data ignored.
       * @return The number of values in th message.
       */
      inline int operator()(const Data& data, const InterfaceInformation& info) const;
    };

    /**
     * @brief Functor for message size caculation for datatypes
     * where at each index can be a variable number of values.
     */
    template<class Data>
    struct MessageSizeCalculator<Data,VariableSize>
    {
      /**
       * @brief Calculate the number of values in message
       *
       * @param info The information about the interface corresponding
       * to the message.
       * @param data A representative of the data we send.
       * @return The number of values in th message.
       */
      inline int operator()(const Data& data, const InterfaceInformation& info) const;
    };
    
    /**
     * @brief Functors for message data gathering.
     */
    template<class Data, class GatherScatter, bool send, typename IndexedTypeFlag>
    struct MessageGatherer
    {};
    
    /**
     * @brief Functor for message data gathering for datatypes
     * where at each index is only one value.
     */
    template<class Data, class GatherScatter, bool send>
    struct MessageGatherer<Data,GatherScatter,send,SizeOne>
    {
      /** @brief The type of the values we send. */
      typedef typename CommPolicy<Data>::IndexedType Type;
      
      /** 
       * @brief The type of the functor that does the actual copying
       * during the data Scattering.
       */
      typedef GatherScatter Gatherer;
      
      enum{
	/** 
	 * @brief The communication mode
	 *
	 *True if this was a forward commuication. 
	 */
	forward=send
      };
      
      /**
       * @brief Copies the values to send into the buffer.
       * @param interface The interface used in the send.
       * @param data The data from which we copy the values.
       * @param buffer The send buffer to copy to.
       * @param bufferSize The size of the buffer in bytes. For checks.
       */
      inline void operator()(const Interface& interface, const Data& data, Type* buffer, size_t bufferSize) const;
    };

    /**
     * @brief Functor for message data scattering for datatypes
     * where at each index can be a variable size of values
     */
    template<class Data, class GatherScatter, bool send>
    struct MessageGatherer<Data,GatherScatter,send,VariableSize>
    {
      /** @brief The type of the values we send. */
      typedef typename CommPolicy<Data>::IndexedType Type;
      
      /** 
       * @brief The type of the functor that does the actual copying
       * during the data Scattering.
       */
      typedef GatherScatter Gatherer;
      
      enum{
	/** 
	 * @brief The communication mode
	 *
	 *True if this was a forward commuication. 
	 */
	forward=send
      };
      
      /**
       * @brief Copies the values to send into the buffer.
       * @param interface The interface used in the send.
       * @param data The data from which we copy the values.
       * @param buffer The send buffer to copy to.
       * @param bufferSize The size of the buffer in bytes. For checks.
       */
      inline void operator()(const Interface& interface, const Data& data, Type* buffer, size_t bufferSize) const;
    };

    /**
     * @brief Functors for message data scattering.
     */
    template<class Data, class GatherScatter, bool send, typename IndexedTypeFlag>
    struct MessageScatterer
    {};

    /**
     * @brief Functor for message data gathering for datatypes
     * where at each index is only one value.
     */
    template<class Data, class GatherScatter, bool send>
    struct MessageScatterer<Data,GatherScatter,send,SizeOne>
    {
      /** @brief The type of the values we send. */
      typedef typename CommPolicy<Data>::IndexedType Type;
      
      /** 
       * @brief The type of the functor that does the actual copying
       * during the data Scattering.
       */
      typedef GatherScatter Scatterer;
      
      enum{
	/** 
	 * @brief The communication mode
	 *
	 *True if this was a forward commuication. 
	 */
	forward=send
      };
      
      /**
       * @brief Copy the message data from the receive buffer to the data.
       * @param interface The interface used in the send.
       * @param data The data to which we copy the values.
       * @param buffer The receive buffer to copy from.
       * @param proc The rank of the process the message is from.
       */
      inline void operator()(const Interface& interface, Data& data, Type* buffer, const int& proc) const;
    };
    /**
     * @brief Functor for message data scattering for datatypes
     * where at each index can be a variable size of values
     */
    template<class Data, class GatherScatter, bool send>
    struct MessageScatterer<Data,GatherScatter,send,VariableSize>
    {
      /** @brief The type of the values we send. */
      typedef typename CommPolicy<Data>::IndexedType Type;
      
      /** 
       * @brief The type of the functor that does the actual copying
       * during the data Scattering.
       */
      typedef GatherScatter Scatterer;
      
      enum{
	/** 
	 * @brief The communication mode
	 *
	 *True if this was a forward commuication. 
	 */
	forward=send
      };
      
      /**
       * @brief Copy the message data from the receive buffer to the data.
       * @param interface The interface used in the send.
       * @param data The data to which we copy the values.
       * @param buffer The receive buffer to copy from.
       * @param proc The rank of the process the message is from.
       */
      inline void operator()(const Interface& interface, Data& data, Type* buffer, const int& proc) const;
    };

    /**
     * @brief Information about a message to send.
     */
    struct MessageInformation
    {
      /** @brief Constructor. */
      MessageInformation()
	: start_(0), size_(0)
      {}
      
      /** 
       * @brief Constructor. 
       * @param start The start of the message in the global buffer.
       * Not in bytes but in number of values from the beginning of
       * the buffer
       * @param size The size of the message in bytes.
      */
      MessageInformation(size_t start, size_t size)
	:start_(start), size_(size)
      {}
      /**
       * @brief Start of the message in the buffer counted in number of value.
       */
      size_t start_;
      /**
       * @brief Number of bytes in the message.
       */
      size_t size_;
    };

    /**
     * @brief Type of the map of information about the messages to send.
     *
     * The key is the process number to communicate with and the key is
     * the pair of information about sending and receiving messages.
     */
    typedef std::map<int,std::pair<MessageInformation,MessageInformation> >
    InformationMap;
    /**
     * @brief Gathered information about the messages to send.
     */
    InformationMap messageInformation_;
    /**
     * @brief Communication buffers.
     */
    char* buffers_[2];
    /**
     * @brief The size of the communication buffers
     */
    size_t bufferSize_[2];
    
    enum{
      /**
       * @brief The tag we use for communication. 
       */
      commTag_
    };
    
    /**
     * @brief The interface we currently work with.
     */
    const Interface* interface_;

    /**
     * @brief Send and receive Data.
     */
    template<class GatherScatter, bool FORWARD, class Data>
    void sendRecv(const Data& source, Data& target);
    
  };
  
  template<class V>
  inline const void* CommPolicy<V>::getAddress(const V& v, int index)
  {
    return &(v[index]);
  }
  
  template<class V>
  inline int CommPolicy<V>::getSize(const V& v, int index)
  {
    return 1;
  }

  template<class K, class A, int n>
  inline const void* CommPolicy<VariableBlockVector<FieldVector<K, n>, A> >::getAddress(const Type& v, int index)
  {
    return &(v[index][0]);
  }

  template<class K, class A, int n>
  inline int CommPolicy<VariableBlockVector<FieldVector<K, n>, A> >::getSize(const Type& v, int index)
  {
    return v[index].getsize();
  }

  
  template<class T>
  inline const typename CopyGatherScatter<T>::IndexedType& CopyGatherScatter<T>::gather(const T& vec, int i)
  {
    return vec[i];
    
  }
  
  template<class T>
  inline void CopyGatherScatter<T>::scatter(T& vec, const IndexedType& v, int i)
  {
    vec[i]=v;
    
  }
  template<typename T>
  DatatypeCommunicator<T>::DatatypeCommunicator()
    : remoteIndices_(0), created_(false)
  {
    requests_[0]=0;
    requests_[1]=0;
  }
  
 

  template<typename T>
  DatatypeCommunicator<T>::~DatatypeCommunicator()
  {
    free();
  }
  
  template<typename T>
  template<class T1, class T2, class V>
  inline void DatatypeCommunicator<T>::build(const RemoteIndices& remoteIndices,
						 const T1& source, V& sendData,
						 const T2& destination, V& receiveData)
  {
    remoteIndices_ = &remoteIndices;
    free();
    createDataTypes<T1,T2,V,false>(source,destination, receiveData);
    createDataTypes<T1,T2,V,true>(source,destination, sendData);
    createRequests<V,true>(sendData, receiveData);
    createRequests<V,false>(receiveData, sendData);
    created_=true;
  }

  template<typename T>
  void DatatypeCommunicator<T>::free()
  {
    if(created_){
      delete[] requests_[0];
      delete[] requests_[1];
      typedef MessageTypeMap::iterator iterator;
      typedef MessageTypeMap::const_iterator const_iterator;
      
      const const_iterator end=messageTypes.end();
      
      for(iterator process = messageTypes.begin(); process != end; ++process){
	MPI_Datatype *type = &(process->second.first);
	MPI_Type_free(type);
	type = &(process->second.second);
	MPI_Type_free(type);
      }
      messageTypes.clear();
      
    }
    
  }
  
  template<typename T>
  template<class T1, class T2, class V, bool send>
  void DatatypeCommunicator<T>::createDataTypes(const T1& sourceFlags, const T2& destFlags, V& data)
  {

    MPIDatatypeInformation<V>  dataInfo(data);
    this->template buildInterface<T1,T2,MPIDatatypeInformation<V>,send>(*remoteIndices_,sourceFlags, destFlags, dataInfo);

    typedef typename RemoteIndices::RemoteIndexMap::const_iterator const_iterator;
    const const_iterator end=this->remoteIndices_->end();
    
    // Allocate MPI_Datatypes and deallocate memory for the type construction.
    for(const_iterator process=this->remoteIndices_->begin(); process != end; ++process){
      IndexedTypeInformation& info=dataInfo.information_[process->first];
      // Shift the displacement
      MPI_Aint base;
      MPI_Address(const_cast<void *>(CommPolicy<V>::getAddress(data, 0)), &base);
      
      for(int i=0; i< info.elements; i++){
	info.displ[i]-=base;
      }
      
      // Create data type
      MPI_Datatype* type = &( send ? messageTypes[process->first].first : messageTypes[process->first].second);
      MPI_Type_hindexed(info.elements, info.length, info.displ, 
		       MPITraits<typename CommPolicy<V>::IndexedType>::getType(),
		       type);
      MPI_Type_commit(type);
      // Deallocate memory
      info.free();
    }
  }
  
  template<typename T>
  template<class V, bool createForward>
  void DatatypeCommunicator<T>::createRequests(V& sendData, V& receiveData)
  {
    typedef std::map<int,std::pair<MPI_Datatype,MPI_Datatype> >::const_iterator MapIterator;
    int rank;
    static int index = createForward?1:0;
    int noMessages = messageTypes.size();
    // allocate request handles
    requests_[index] = new MPI_Request[2*noMessages];
    const MapIterator end = messageTypes.end();
    int request=0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Set up the requests for receiving first
    for(MapIterator process = messageTypes.begin(); process != end; 
	++process, ++request){
      MPI_Datatype type = createForward ? process->second.second : process->second.first;
      void* address = const_cast<void*>(CommPolicy<V>::getAddress(receiveData,0));
      MPI_Recv_init(address, 1, type, process->first, commTag_, this->remoteIndices_->communicator(), requests_[index]+request);
    }
    
    // And now the send requests

    for(MapIterator process = messageTypes.begin(); process != end; 
	++process, ++request){
      MPI_Datatype type = createForward ? process->second.first : process->second.second;
      void* address =  const_cast<void*>(CommPolicy<V>::getAddress(sendData, 0));
      MPI_Ssend_init(address, 1, type, process->first, commTag_, this->remoteIndices_->communicator(), requests_[index]+request);
    }
  }   
	
  template<typename T>
  void DatatypeCommunicator<T>::forward()
  {
    sendRecv(requests_[1]);
  }
  
  template<typename T>
  void DatatypeCommunicator<T>::backward()
  {
    sendRecv(requests_[0]);
  }

  template<typename T>
  void DatatypeCommunicator<T>::sendRecv(MPI_Request* requests)
  {
    int noMessages = messageTypes.size();
    // Start the receive calls first
    MPI_Startall(noMessages, requests);
    // Now the send calls
    MPI_Startall(noMessages, requests+noMessages);
    
    // Wait for completion of the communication send first then receive
    MPI_Status* status=new MPI_Status[2*noMessages];
    for(int i=0; i<2*noMessages; i++)
      status[i].MPI_ERROR=MPI_SUCCESS;
    
    int send = MPI_Waitall(noMessages, requests+noMessages, status+noMessages);
    int receive = MPI_Waitall(noMessages, requests, status);
    
    // Error checks
    int success=1, globalSuccess=0;
    if(send==MPI_ERR_IN_STATUS){
      int rank;
      MPI_Comm_rank(this->remoteIndices_->communicator(), &rank);
      std::cerr<<rank<<": Error in sending :"<<std::endl;
      // Search for the error
      for(int i=noMessages; i< 2*noMessages; i++)
	if(status[i].MPI_ERROR!=MPI_SUCCESS){
	  char message[300];
	  int messageLength;
	  MPI_Error_string(status[i].MPI_ERROR, message, &messageLength);
	  std::cerr<<" source="<<status[i].MPI_SOURCE<<" message: ";
	  for(int i=0; i< messageLength; i++)
	    std::cout<<message[i];
	}
      std::cerr<<std::endl;
      success=0;
    }
    
    if(receive==MPI_ERR_IN_STATUS){
      int rank;
      MPI_Comm_rank(this->remoteIndices_->communicator(), &rank);
      std::cerr<<rank<<": Error in receiving!"<<std::endl;
      // Search for the error
      for(int i=0; i< noMessages; i++)
	if(status[i].MPI_ERROR!=MPI_SUCCESS){
	  char message[300];
	  int messageLength;
	  MPI_Error_string(status[i].MPI_ERROR, message, &messageLength);
	  std::cerr<<" source="<<status[i].MPI_SOURCE<<" message: ";
	  for(int i=0; i< messageLength; i++)
	    std::cerr<<message[i];
	}
      std::cerr<<std::endl;
      success=0;
    }

    MPI_Allreduce(&success, &globalSuccess, 1, MPI_INT, MPI_MIN, this->remoteIndices_->communicator());

    if(!globalSuccess)
      DUNE_THROW(CommunicationError, "A communication error occurred!");
    
      
  }
  


  template<typename T>
  BufferedCommunicator<T>::BufferedCommunicator()
    : interface_(0)
  {
    buffers_[0]=0;
    buffers_[1]=0;
    bufferSize_[0]=0;
    bufferSize_[1]=0;
  }
  
  
  template<typename T>
  template<class Data>
  typename EnableIf<is_same<SizeOne, typename CommPolicy<Data>::IndexedTypeFlag>::value, void>::Type
  BufferedCommunicator<T>::build(const Interface& interface)
  {
    typedef typename Interface::InformationMap::const_iterator const_iterator;
    typedef typename CommPolicy<Data>::IndexedTypeFlag Flag;
    const const_iterator end = interface.interfaces().end();
    int lrank;
    MPI_Comm_rank(interface.communicator(), &lrank);
    
    bufferSize_[0]=0;
    bufferSize_[1]=0;

    for(const_iterator interfacePair = interface.interfaces().begin();
	interfacePair != end; ++interfacePair){  
      int noSend = MessageSizeCalculator<Data,Flag>()(interfacePair->second.first);
      int noRecv = MessageSizeCalculator<Data,Flag>()(interfacePair->second.second);
      messageInformation_.insert(std::make_pair(interfacePair->first,
				 std::make_pair(MessageInformation(bufferSize_[0],
								   noSend*sizeof(typename CommPolicy<Data>::IndexedType)),
						MessageInformation(bufferSize_[1],
								   noRecv*sizeof(typename CommPolicy<Data>::IndexedType)))));
      bufferSize_[0] += noSend;
      bufferSize_[1] += noRecv;
    }

    // allocate the buffers
    bufferSize_[0] *= sizeof(typename CommPolicy<Data>::IndexedType);
    bufferSize_[1] *= sizeof(typename CommPolicy<Data>::IndexedType);
    
    buffers_[0] = new char[bufferSize_[0]];
    buffers_[1] = new char[bufferSize_[1]];
    interface_ = &interface;
    
  }  
  
  template<typename T>
  template<class Data>
  void BufferedCommunicator<T>::build(const Data& source, const Data& dest, const Interface& interface)
  {
    typedef typename Interface::InformationMap::const_iterator const_iterator;
    typedef typename CommPolicy<Data>::IndexedTypeFlag Flag;
    const const_iterator end = interface.interfaces().end();
    
    bufferSize_[0]=0;
    bufferSize_[1]=0;

    for(const_iterator interfacePair = interface.interfaces().begin();
	interfacePair != end; ++interfacePair){      
      int noSend = MessageSizeCalculator<Data,Flag>()(source, interfacePair->second.first);
      int noRecv = MessageSizeCalculator<Data,Flag>()(dest, interfacePair->second.second);
      
      messageInformation_.insert(std::make_pair(interfacePair->first,
						std::make_pair(MessageInformation(bufferSize_[0],
										  noSend*sizeof(typename CommPolicy<Data>::IndexedType)),
							       MessageInformation(bufferSize_[1],
										  noRecv*sizeof(typename CommPolicy<Data>::IndexedType)))));
      bufferSize_[0] += noSend;
      bufferSize_[1] += noRecv;
    }

    bufferSize_[0] *= sizeof(typename CommPolicy<Data>::IndexedType);
    bufferSize_[1] *= sizeof(typename CommPolicy<Data>::IndexedType);
    // allocate the buffers
    buffers_[0] = new char[bufferSize_[0]];
    buffers_[1] = new char[bufferSize_[1]];
    interface_ = &interface;
  }
  
  template<typename T>
  void BufferedCommunicator<T>::free()
  {
    if(interface_!=0){
      messageInformation_.clear();
      delete[] buffers_[0];
      delete[] buffers_[1];
      interface_=0;
    }
  }

  template<typename T>
  BufferedCommunicator<T>::~BufferedCommunicator()
  {
    free();
  }
  
  template<typename T>
  template<class Data>
  inline int BufferedCommunicator<T>::MessageSizeCalculator<Data,SizeOne>::operator()
    (const InterfaceInformation& info) const
  {
    return info.size();
  }

  template<typename T>
  template<class Data>
  inline int BufferedCommunicator<T>::MessageSizeCalculator<Data,SizeOne>::operator()
    (const Data& data, const InterfaceInformation& info) const
  {
    return operator()(info);
  }

  template<typename T>
  template<class Data>
  inline int BufferedCommunicator<T>::MessageSizeCalculator<Data, VariableSize>::operator()
    (const Data& data, const InterfaceInformation& info) const
  {
    int entries=0;

    for(int i=0; i <  info.size(); i++)
      entries += CommPolicy<Data>::getSize(data,info[i]);

    return entries;
  }
  
  template<typename T>
  template<class Data, class GatherScatter, bool FORWARD>
  inline void BufferedCommunicator<T>::MessageGatherer<Data,GatherScatter,FORWARD,VariableSize>::operator()(const Interface& interface,const Data& data, Type* buffer, size_t bufferSize) const
  {
    typedef typename Interface::InformationMap::const_iterator
      const_iterator;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const const_iterator end = interface.interfaces().end();
    size_t index=0;
    
    for(const_iterator interfacePair = interface.interfaces().begin();
	interfacePair != end; ++interfacePair){
      int size = forward ? interfacePair->second.first.size() :
	interfacePair->second.second.size();
      int proc = interfacePair->first();
      
      for(int i=0; i < size; i++){  
	int local = forward ? interfacePair->second->first[i] :
	  interfacePair->second->second[i];
	for(int j=0; j < CommPolicy<Data>::getSize(data, local);j++, index++){

#ifdef DUNE_ISTL_WITH_CHECKING
	  assert(bufferSize>=(index+1)*sizeof(typename CommPolicy<Data>::IndexedType));
#endif
	  buffer[index]=GatherScatter::gather(data, local, j);
	}
	
      }
    }
	
  }

  template<typename T>
  template<class Data, class GatherScatter, bool FORWARD>
  inline void BufferedCommunicator<T>::MessageGatherer<Data,GatherScatter,FORWARD,SizeOne>::operator()(const Interface& interface, const Data& data, Type* buffer, size_t bufferSize)const
  {
    typedef typename Interface::InformationMap::const_iterator
      const_iterator;
    const const_iterator end = interface.interfaces().end();
    size_t index = 0;
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(const_iterator interfacePair = interface.interfaces().begin();
	interfacePair != end; ++interfacePair){
      size_t size = FORWARD ? interfacePair->second.first.size() :
	interfacePair->second.second.size();
      
      for(size_t i=0; i < size; i++){

#ifdef DUNE_ISTL_WITH_CHECKING
	  assert(bufferSize>=(index+1)*sizeof(typename CommPolicy<Data>::IndexedType));
#endif

	buffer[index++] = GatherScatter::gather(data, FORWARD ? interfacePair->second.first[i] :
						interfacePair->second.second[i]);
      }
    }
	
  }
  
  template<typename T>
  template<class Data, class GatherScatter, bool FORWARD>
  inline void BufferedCommunicator<T>::MessageScatterer<Data,GatherScatter,FORWARD,VariableSize>::operator()(const Interface& interface, Data& data, Type* buffer, const int& proc)const
  {
    typedef typename Interface::Information Information;
    const typename Interface::InformationMap::const_iterator infoPair = interface.interfaces().find(proc);

    assert(infoPair!=interface.interfaces().end());
    
    const Information& info = FORWARD ? infoPair->second.second :
       infoPair->second.first;

    for(int i=0, index=0; i < info.size(); i++){
	for(int j=0; j < CommPolicy<Data>::getSize(data, info[i]); j++)
	  GatherScatter::scatter(data, buffer[index++], info[i], j);
    }
  }

  template<typename T>
  template<class Data, class GatherScatter, bool FORWARD>
  inline void BufferedCommunicator<T>::MessageScatterer<Data,GatherScatter,FORWARD,SizeOne>::operator()(const Interface& interface, Data& data, Type* buffer, const int& proc)const
  {
    typedef typename Interface::Information Information;
    const typename Interface::InformationMap::const_iterator infoPair = interface.interfaces().find(proc);

    assert(infoPair!=interface.interfaces().end());
    
    const Information& info = FORWARD ? infoPair->second.second :
      infoPair->second.first;
    
    for(size_t i=0; i < info.size(); i++){
      GatherScatter::scatter(data, buffer[i], info[i]);
    }
  }
  
  template<typename T>
  template<class GatherScatter,class Data>
  void BufferedCommunicator<T>::forward(Data& data)
  {
    this->template sendRecv<GatherScatter,true>(data, data);
  }
  
  template<typename T>
  template<class GatherScatter, class Data>
  void BufferedCommunicator<T>::backward(Data& data)
  {
    this->template sendRecv<GatherScatter,false>(data, data);
  }

  template<typename T>
  template<class GatherScatter, class Data>
  void BufferedCommunicator<T>::forward(const Data& source, Data& dest)
  {
    this->template sendRecv<GatherScatter,true>(source, dest);
  }
  
  template<typename T>
  template<class GatherScatter, class Data>
  void BufferedCommunicator<T>::backward(Data& source, const Data& dest)
  {
    this->template sendRecv<GatherScatter,false>(dest, source);
  }

  template<typename T>
  template<class GatherScatter, bool FORWARD, class Data>
  void BufferedCommunicator<T>::sendRecv(const Data& source, Data& dest) 
  {
    int rank, lrank;
    
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_rank(MPI_COMM_WORLD,&lrank);
    
    typedef typename CommPolicy<Data>::IndexedType Type;
    Type *sendBuffer, *recvBuffer;
    size_t sendBufferSize, recvBufferSize;
    
    if(FORWARD){
      sendBuffer = reinterpret_cast<Type*>(buffers_[0]);
      sendBufferSize = bufferSize_[0];
      recvBuffer = reinterpret_cast<Type*>(buffers_[1]);
      recvBufferSize = bufferSize_[1];
    }else{
      sendBuffer = reinterpret_cast<Type*>(buffers_[1]);
      sendBufferSize = bufferSize_[1];
      recvBuffer = reinterpret_cast<Type*>(buffers_[0]);
      recvBufferSize = bufferSize_[0];
    }
    typedef typename CommPolicy<Data>::IndexedTypeFlag Flag;

    MessageGatherer<Data,GatherScatter,FORWARD,Flag>()(*interface_, source, sendBuffer, sendBufferSize);
    
    MPI_Request* sendRequests = new MPI_Request[messageInformation_.size()];
    MPI_Request* recvRequests = new MPI_Request[messageInformation_.size()];
    
    // Setup receive first
    typedef typename InformationMap::const_iterator const_iterator;

    const const_iterator end = messageInformation_.end();
    size_t i=0;
    int* processMap = new int[messageInformation_.size()];
    
    for(const_iterator info = messageInformation_.begin(); info != end; ++info, ++i){
	  processMap[i]=info->first;
	  if(FORWARD){
	    assert(info->second.second.start_*sizeof(typename CommPolicy<Data>::IndexedType)+info->second.second.size_ <= recvBufferSize );
	    MPI_Irecv(recvBuffer+info->second.second.start_, info->second.second.size_,
		      MPI_BYTE, info->first, commTag_, interface_->communicator(),
		      recvRequests+i);
	  }else{
	    assert(info->second.first.start_*sizeof(typename CommPolicy<Data>::IndexedType)+info->second.first.size_ <= recvBufferSize );
	    MPI_Irecv(recvBuffer+info->second.first.start_, info->second.first.size_,
		      MPI_BYTE, info->first, commTag_, interface_->communicator(),
		      recvRequests+i);
	  }
    }
    
    // now the send requests
    i=0;
    for(const_iterator info = messageInformation_.begin(); info != end; ++info, ++i)
      if(FORWARD){
	assert(info->second.first.start_*sizeof(typename CommPolicy<Data>::IndexedType)+info->second.first.size_ <= sendBufferSize );
	MPI_Issend(sendBuffer+info->second.first.start_, info->second.first.size_,
		   MPI_BYTE, info->first, commTag_, interface_->communicator(),
		   sendRequests+i);
      }else{
	assert(info->second.second.start_*sizeof(typename CommPolicy<Data>::IndexedType)+info->second.second.size_ <= sendBufferSize );
	MPI_Issend(sendBuffer+info->second.second.start_, info->second.second.size_,
		   MPI_BYTE, info->first, commTag_, interface_->communicator(),
		   sendRequests+i);
      }
    
    // Wait for completion of receive and immediately start scatter
    i=0;
    int success = 1;
    int finished = MPI_UNDEFINED;
    MPI_Status status;//[messageInformation_.size()];
    //MPI_Waitall(messageInformation_.size(), recvRequests, status);
    
    for(i=0;i< messageInformation_.size();i++){
      MPI_Waitany(messageInformation_.size(), recvRequests, &finished, &status);
      assert(finished != MPI_UNDEFINED);
      
      if(status.MPI_ERROR==MPI_SUCCESS){
	int& proc = processMap[finished];
	typename InformationMap::const_iterator infoIter = messageInformation_.find(proc);
	assert(infoIter != messageInformation_.end());

	MessageInformation info = (FORWARD)? infoIter->second.second : infoIter->second.first;
	assert(info.start_+info.size_ <= recvBufferSize);

	MessageScatterer<Data,GatherScatter,FORWARD,Flag>()(*interface_, dest, recvBuffer+info.start_, proc);
      }else{
	std::cerr<<rank<<": MPI_Error occurred while receiving message from "<<processMap[finished]<<std::endl;
	success=0;
      }
    }

    MPI_Status recvStatus;
    
    // Wait for completion of sends
    for(i=0;i< messageInformation_.size();i++)
      if(MPI_SUCCESS!=MPI_Wait(sendRequests+i, &recvStatus)){
	std::cerr<<rank<<": MPI_Error occurred while sending message to "<<processMap[finished]<<std::endl;
	success=0;
      }
    int globalSuccess;
    MPI_Allreduce(&success, &globalSuccess, 1, MPI_INT, MPI_MIN, interface_->communicator());

    if(!globalSuccess)
      DUNE_THROW(CommunicationError, "A communication error occurred!");
	
    delete[] processMap;
    delete[] sendRequests;
    delete[] recvRequests;

  }

  /** @} */
}

#endif
