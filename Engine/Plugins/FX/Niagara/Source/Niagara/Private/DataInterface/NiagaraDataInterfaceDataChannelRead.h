// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once


/**

Data Channel Read Interface.
Enabled Niagara Systems to read data from a NiagaraDataChannel that has been previously generated by another System or Game code/BP.

Read DIs will grab a buffer from the handler for their bound Data Channel.
This could be a global buffer or some localized buffer for their region of the map
Or any other buffer sub division scheme that the handler chooses.
The main point being that this buffer can change from frame to frame and instance to instance.
The handler is free to store and distribute it's Channel Data as it pleases.

Accessor functions on the Data Channel Read and Write DIs can have any number of parameters, allowing a single function call to access arbitrary data from the Channel.
This avoids cumbersome work in the graph to access data but requires special handling inside the DI.

TODO: Local support - We can read directly from a writer DI in the same system to avoid having to publish that data in some cases. 
There is working code for this already but the concept/API needs more fleshing out before we include it.

*/


#include "NiagaraDataInterfaceDataChannelCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceDataChannelRead.generated.h"

class UNiagaraDataInterfaceDataChannelWrite;
struct FNDIDataChannelWriteInstanceData;
class UNiagaraDataChannelHandler;
class FNiagaraDataBuffer;

UCLASS(Experimental, EditInlineNew, Category = "Data Channels", meta = (DisplayName = "Data Channel Reader"))
class NIAGARA_API UNiagaraDataInterfaceDataChannelRead : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()
protected:

public:

	//TODO: In future we may allow reads and writes that stay local to a single system.	
// 	/** The scope at which to read data. When reading locally we'll read directly from a Data Channel Write Interface in this or another emitter. When reading from World scope, we'll read from a named Data Channel. */
// 	UPROPERTY(EditAnywhere, Category = "Data Channel")
// 	ENiagaraDataChannelScope Scope;
// 
// 	/** Name of the source */
// 	UPROPERTY(EditAnywhere, Category="Data Channel", meta=(EditCondition = "Scope == ENiagaraDataChannelScope::Local"))
// 	FName Source;

	/** When reading from external, the channel to consume. */
	UPROPERTY(EditAnywhere, Category="Data Channel")
	FNiagaraDataChannelReference Channel;
	
	/** True if this reader will read the current frame's data. If false, we read the previous frame.
	* Reading the current frame introduces a tick order dependency but allows for zero latency reads. Any data channel elements that are generated after this reader is used are missed.
	* Reading the previous frame's data introduces a frame of latency but ensures we never miss any data as we have access to the whole frame.
	*/
	UPROPERTY(EditAnywhere, Category = "Data Channel")
	bool bReadCurrentFrame = false;

	//UObject Interface
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	//UObject Interface End

	//UNiagaraDataInterface Interface
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc) override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return true; }
#if WITH_EDITORONLY_DATA
	virtual bool AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const override;
	virtual void GetCommonHLSL(FString& OutHLSL)override;
	virtual bool GetFunctionHLSL(FNiagaraDataInterfaceHlslGenerationContext& HlslGenContext, FString& OutHLSL) override;
	virtual void GetParameterDefinitionHLSL(FNiagaraDataInterfaceHlslGenerationContext& HlslGenContext, FString& OutHLSL) override;

	virtual void PostCompile()override;
#endif	

#if WITH_EDITOR	
	virtual void GetFeedback(UNiagaraSystem* InAsset, UNiagaraComponent* InComponent, TArray<FNiagaraDataInterfaceError>& OutErrors, TArray<FNiagaraDataInterfaceFeedback>& OutWarnings, TArray<FNiagaraDataInterfaceFeedback>& OutInfo) override;
	virtual void ValidateFunction(const FNiagaraFunctionSignature& Function, TArray<FText>& OutValidationErrors) override;
#endif

	virtual bool Equals(const UNiagaraDataInterface* Other) const override;

	virtual void BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;
	virtual void SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;

	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual int32 PerInstanceDataSize() const override;
	virtual bool HasPreSimulateTick() const override { return true; }
	virtual bool HasPostSimulateTick() const override { return true; }
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;
	virtual void GetEmitterDependencies(UNiagaraSystem* Asset, TArray<FVersionedNiagaraEmitter>& Dependencies) const;
	//UNiagaraDataInterface Interface

	//Functions usable anywhere.
	void Num(FVectorVMExternalFunctionContext& Context);
	void Read(FVectorVMExternalFunctionContext& Context, int32 FuncIdx);
	void Consume(FVectorVMExternalFunctionContext& Context, int32 FuncIdx);
	
	FNDIDataChannelCompiledData& GetCompiledData() { return CompiledData; }

protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
	UNiagaraDataInterfaceDataChannelWrite* FindSourceDI()const;

 	UPROPERTY()
 	FNDIDataChannelCompiledData CompiledData;
};

struct FNDIDataChannelReadInstanceData
{
	//TODO: Local reads.
// 	/** The local DataChannel writer we're bounds to, if any. */
// 	TWeakObjectPtr<UNiagaraDataInterfaceDataChannelWrite> SourceDI;
// 
// 	/** The instance data of the local DataChannel writer we're bound to, if any. */
// 	FNDIDataChannelWriteInstanceData* SourceInstData = nullptr;
	
	/** Pointer to the world DataChannel Channel we're reading from, if any. */
	TWeakObjectPtr<UNiagaraDataChannelHandler> DataChannel;

	/** Pointer to the cpu simulation buffer from the DataChannel if we're reading from there. */
	FNiagaraDataBuffer* ExternalCPU = nullptr;
	
	/** The GPU data set from the DataChannel. We must grab the whole set rather than a buffer here as we don't know which buffer will be contain this frame's channel data. 
	*	TODO: We'll likely want to improve this GPU sim flow soon. Possibly grab an RT proxy object from the data channel handler and pass that over to the GPU instead of a direct dataset like this.
	*/
	FNiagaraDataSet* ExternalGPU = nullptr;

	/** Cached hash to check if the layout of our source data has changed. */
	uint64 ChachedDataSetLayoutHash = INDEX_NONE;

	/** When true we should update our RT data next tick. */
	mutable bool bUpdateRTData = false;

	/** Binding info from the Data Channel to each of our function's parameters. */
	//FNDIDataChannelBindingInfo BindingInfo;

	TArray<FNDIDataChannel_FuncToDataSetBindingPtr, TInlineAllocator<8>> FuncToDataSetBindingInfo;

	/** Keys for each of the above function infos. */
	//TArray<uint32> FuncToDataSetLayoutKeys;
	
	/** Index we use to consume DataChannel data.
		Reset at each tick.
		All consumers of the DI use the same index.
		TODO: Allow ConsumeUnique that has a unique counter per call.
	*/
	std::atomic<int32> ConsumeIndex = 0;

	virtual ~FNDIDataChannelReadInstanceData();
	FNiagaraDataBuffer* GetReadBufferCPU();
	bool Init(UNiagaraDataInterfaceDataChannelRead* Interface, FNiagaraSystemInstance* Instance);
	bool Tick(UNiagaraDataInterfaceDataChannelRead* Interface, FNiagaraSystemInstance* Instance);
	bool PostTick(UNiagaraDataInterfaceDataChannelRead* Interface, FNiagaraSystemInstance* Instance);
};

struct FNiagaraDataInterfaceProxy_DataChannelRead : public FNiagaraDataInterfaceProxy
{
	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override;

	/** Persistent per instance data on the RT. Constructed when consuming data passed from GT->RT. */
	struct FInstanceData
	{
		//GPU Dataset from the channel handler. We'll grab the current buffer from this on the RT.
		//This must be grabbed fresh from the handler each frame as it's lifetime cannot be ensured.
		FNiagaraDataSet* ChannelDataSet = nullptr;

		/**
		A buffer containing layout information needed to access parameters for each script using this DI.
		*/
		FReadBuffer ParameterLayoutBuffer;

		TResourceArray<uint32> ParameterLayoutData;

		/**
		Offsets into the parameter table are embedded in the gpu script hlsl.
		At hlsl gen time we can only know which parameter are accessed by each script individually so each script must have it's own parameter binding table.
		*/
		TMap<FNiagaraCompileHash, uint32> GPUScriptParameterTableOffsets;
	};

	TMap<FNiagaraSystemInstanceID, FInstanceData> SystemInstancesToProxyData_RT;
};