#pragma once
#include "CoreMinimal.h"


template<typename T>
class CustomQueue{
    private:
    struct Node{
        T elem;
        Node* next;
        Node(T val):elem(val),next(nullptr){}
    };
    
    Node* FrontNode;
    Node* RearNode;
    int32 Size;  
    
public:
    CustomQueue():FrontNode(nullptr), RearNode(nullptr), Size(0){}
    ~CustomQueue(){
        Clear();
    }
    bool IsEmpty(){
        return (FrontNode==nullptr);
    }
    void Clear(){
        while(!IsEmpty()){
            Dequeue();
        }
    }

    T Front(){
        if(IsEmpty()){
            UE_LOG(LogTemp,Error, TEXT("Empty Queue!"));
            return T();
        }
        return FrontNode->elem;
    }
   
    void Enqueue(T val){
        Node* NewNode = new Node(val);
        if(IsEmpty()){
            FrontNode = RearNode = NewNode;
        }
        else{
            RearNode->next=NewNode;
            RearNode= NewNode;
        }
        ++Size;
    }
    void Dequeue(){
        if(IsEmpty()){
            UE_LOG(LogTemp, Error, TEXT("Queue is Empty!"));
            return;
        }
        Node* OldNode = FrontNode;
        FrontNode=FrontNode->next;
        if(FrontNode==nullptr){
            FrontNode=RearNode=nullptr;
        }
        delete OldNode;
        Size--;
    }
    
   
    int32 GetSize() const
    {
        return Size;
    }
    

    void Print() const
    {
        if (IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Queue is empty"));
            return;
        }
        
        Node* Current = FrontNode;
        FString QueueContents = TEXT("Queue: ");
        
        while (Current != nullptr)
        {
            QueueContents += TEXT("[Node] -> ");
            Current = Current->next;
        }
        
        QueueContents += TEXT("NULL");
        UE_LOG(LogTemp, Warning, TEXT("%s"), *QueueContents);
    }
};
