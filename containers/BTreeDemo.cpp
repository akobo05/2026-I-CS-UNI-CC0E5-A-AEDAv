//#include <iostream.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "../types.h"
#include "BTree.h"
#include "traits.h"

//const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const TypeBTree * keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";

const T1 BTreeSize = 3;

void ImprimirClave(tagObjectInfo< AscBTreeTrait<TypeBTree> >& info, T1 nivel) { cout << info.key << " "; }
flag EsVocal(tagObjectInfo< AscBTreeTrait<TypeBTree> >& info, T1 nivel) {
    TypeBTree k = info.key;
    return (k=='A'||k=='E'||k=='I'||k=='O'||k=='U'||k=='a'||k=='e'||k=='i'||k=='o'||k=='u');
}

void DemoBTree()
{
       T1 result, i;
       BTree< AscBTreeTrait<TypeBTree> > bt (BTreeSize);
       for (i = 0; keys1[i]; i++)
       {
               //cout<<"Inserting "<<keys1[i]<<endl;
               result = bt.Insert(keys1[i], i*i);
               //bt.Print(cout);
       }
       cout << bt;

       cout << "\nForEach:\n";
       bt.ForEach(ImprimirClave);
       cout << endl;

       {
               auto *pVocal = bt.FirstThat(EsVocal);
               if( pVocal )
                       cout << "FirstThat (vocal): " << pVocal->key << " -> " << pVocal->ObjID << endl;
               else
                       cout << "FirstThat (vocal): no encontrado" << endl;
       }

       // Variante descendente: el comparador viene del Trait (DescBTreeTrait)
       cout << "\nDescendente:\n";
       BTree< DescBTreeTrait<TypeBTree> > btDesc (BTreeSize);
       for (i = 0; keys1[i]; i++)
               btDesc.Insert(keys1[i], i*i);
       cout << btDesc;

       // operator>> : construye un arbol leyendo claves de un stream
       cout << "\noperator>> (lee 'M E X I C O'):\n";
       BTree< AscBTreeTrait<TypeBTree> > btIn (BTreeSize);
       istringstream entrada("M E X I C O");
       entrada >> btIn;
       cout << btIn;
}
