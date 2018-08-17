
/*
 * List Management Package
 * 
 * David Harrison
 * University of California, Berkeley, 1985
 *
 * This package implements a simple generic linked list data type.  It
 * uses a doubly linked list structure and provides some standard operations
 * for storing and retrieving data from the list.
 */

#include "util.h"
#include "../include/list.h"        /* Self declaration        */

#define alloc(type)    (type *) malloc(sizeof(type))

/*
 * The list identifier is in reality a pointer to the following list
 * descriptor structure.  Lists are doubly linked with both top and
 * bottom pointers stored in the list descriptor.  The length
 * of the list is also stored in the descriptor.
 */

typedef struct list_elem {    /* One list element  */
    struct list_desc *mainList;    /* List descriptor   */
    struct list_elem *prevPtr;    /* Previous element  */
    struct list_elem *nextPtr;    /* Next list element */
    lsGeneric        userData;        /* User pointer      */
} lsElem;

typedef struct list_desc {    /* List descriptor record            */
    lsElem *topPtr, *botPtr;    /* Pointer to top and bottom of list */
    int    length;            /* Length of list                    */
} lsDesc;


/*
 * Generators are in reality pointers to the generation descriptor 
 * defined below.  A generator has a current spot which is *between*
 * two items.  Thus,  a generator consists of two pointers:  record
 * before spot and record after spot.  A pointer to the main list
 * is included so the top and bottom pointers of the list can be
 * modified if needed.
 */

typedef struct gen_desc {    /* Generator Descriptor 	*/
    lsDesc *mainList;        /* Pointer to list descriptor   */
    lsElem *beforeSpot;        /* Item before the current spot */
    lsElem *afterSpot;        /* Item after the current spot  */
} lsGenInternal;

/*
 * Handles are in reality pointers to lsElem records.  They are
 * cheap to generate and need not be disposed.
 */



/*
 * List Creation and Deletion
 */

lsList lsCreate()
/*
 * Creates a new linked list and returns its handle.  The handle is used
 * by all other list manipulation routines and should not be discarded.
 */
{
    lsDesc *newList;

    newList = alloc(lsDesc);
    newList->topPtr = newList->botPtr = NIL(lsElem);
    newList->length = 0;
    return ((lsList) newList);
}

lsStatus lsDestroy(list, delFunc)
        lsList list;            /* List to destroy              */
        void (*delFunc)();        /* Routine to release user data */
/*
 * Frees all resources associated with the specified list.  It frees memory
 * associated with all elements of the list and then deletes the list.
 * User data is released by calling 'delFunc' with the pointer as the
 * argument.  Accessing a list after its destruction is a no-no.
 */
{
    lsDesc *realList;
    lsElem *index, *temp;

    realList = (lsDesc *) list;
    /* Get rid of elements */
    index    = realList->topPtr;
    while (index != NIL(lsElem)) {
        temp = index;
        index = index->nextPtr;
        if (delFunc)
            (*delFunc)(temp->userData);
        free((lsGeneric) temp);
    }
    /* Get rid of descriptor */
    free((lsGeneric) realList);
    return (LS_OK);
}


/*
 * Copying lists
 */

static lsGeneric lsIdentity(data)
        lsGeneric data;
/* Identity copy function */
{
    return data;
}

lsList lsCopy(list, copyFunc)
        lsList list;            /* List to be copied         */
        lsGeneric (*copyFunc)();    /* Routine to copy user data */
/*
 * Returns a copy of list `list'.  If `copyFunc' is non-zero,
 * it will be called for each item in `list' and the pointer it 
 * returns will be used in place of the original user data for the 
 * item in the newly created list.  The form of `copyFunc' should be:
 *   lsGeneric copyFunc(data)
 *   lsGeneric data;
 * This is normally used to make copies of the user data in the new list.
 * If no `copyFunc' is provided,  an identity function is used.
 */
{
    lsList    newList;
    lsGen     gen;
    lsGeneric data;

    if (!copyFunc) copyFunc = lsIdentity;
    newList = lsCreate();
    gen     = lsStart(list);
    while (lsNext(gen, &data, LS_NH) == LS_OK) {
        (void) lsNewEnd(newList, (*copyFunc)(data), LS_NH);
    }
    lsFinish(gen);
    return newList;
}


/*
 * Adding New Elements to the Beginning and End of a List
 */

lsStatus lsNewBegin(list, data, itemHandle)
        lsList list;            /* List to add element to    */
        lsGeneric data;            /* Arbitrary pointer to data */
        lsHandle *itemHandle;        /* Handle to data (returned) */
/*
 * Adds a new item to the start of a previously created linked list.
 * If 'itemHandle' is non-zero,  it will be filled with a handle
 * which can be used to generate a generator positioned at the
 * item without generating through the list.
 */
{
    lsDesc *realList = (lsDesc *) list;
    lsElem *newElem;

    newElem = alloc(lsElem);
    newElem->userData = data;
    newElem->nextPtr  = realList->topPtr;
    newElem->prevPtr  = NIL(lsElem);
    newElem->mainList = realList;
    if (realList->topPtr == NIL(lsElem)) {
        /* The new item is both the top and bottom element */
        realList->botPtr = newElem;
    } else {
        /* There was a top element - make its prev correct */
        realList->topPtr->prevPtr = newElem;
    }
    realList->topPtr  = newElem;
    realList->length += 1;
    if (itemHandle) *itemHandle = (lsHandle) newElem;
    return (LS_OK);
}

lsStatus lsNewEnd(list, data, itemHandle)
        lsList list;            /* List to append element to */
        lsGeneric data;            /* Arbitrary pointer to data */
        lsHandle *itemHandle;        /* Handle to data (returned) */
/*
 * Adds a new item to the end of a previously created linked list.
 * This routine appends the item in constant time and
 * can be used freely without guilt.
 */
{
    lsDesc *realList = (lsDesc *) list;
    lsElem *newElem;

    newElem = alloc(lsElem);
    newElem->userData             = data;
    newElem->prevPtr              = realList->botPtr;
    newElem->nextPtr              = NIL(lsElem);
    newElem->mainList             = realList;
    if (realList->topPtr == NIL(lsElem))
        realList->topPtr          = newElem;
    if (realList->botPtr != NIL(lsElem))
        realList->botPtr->nextPtr = newElem;
    realList->botPtr              = newElem;
    realList->length += 1;
    if (itemHandle) *itemHandle = (lsHandle) newElem;
    return (LS_OK);
}

/*
 * Retrieving the first and last items of a list
 */

lsStatus lsFirstItem(list, data, itemHandle)
        lsList list;            /* List to get item from */
        lsGeneric *data;            /* User data (returned)  */
        lsHandle *itemHandle;    /* Handle to data (returned) */
/*
 * Returns the first item in the list.  If the list is empty,
 * it returns LS_NOMORE.  Otherwise,  it returns LS_OK.
 * If 'itemHandle' is non-zero,  it will be filled with a
 * handle which may be used to generate a generator.
 */
{
    lsDesc *realList = (lsDesc *) list;

    if (realList->topPtr != NIL(lsElem)) {
        *data                       = realList->topPtr->userData;
        if (itemHandle) *itemHandle = (lsHandle) (realList->topPtr);
        return (LS_OK);
    } else {
        *data                       = (lsGeneric) 0;
        if (itemHandle) *itemHandle = (lsHandle) 0;
        return (LS_NOMORE);
    }
}

lsStatus lsLastItem(list, data, itemHandle)
        lsList list;            /* List to get item from */
        lsGeneric *data;            /* User data (returned)  */
        lsHandle *itemHandle;    /* Handle to data (returned) */
/*
 * Returns the last item of a list.  If the list is empty,
 * the routine returns LS_NOMORE.  Otherwise,  'data' will
 * be set to the last item and the routine will return LS_OK.
 * If 'itemHandle' is non-zero,  it will be filled with a
 * handle which can be used to generate a generator postioned
 * at this item.
 */
{
    lsDesc *realList = (lsDesc *) list;

    if (realList->botPtr != NIL(lsElem)) {
        *data                       = realList->botPtr->userData;
        if (itemHandle) *itemHandle = (lsHandle) (realList->botPtr);
        return (LS_OK);
    } else {
        *data                       = (lsGeneric) 0;
        if (itemHandle) *itemHandle = (lsHandle) 0;
        return (LS_NOMORE);
    }
}



/* Length of a list */

int lsLength(list)
        lsList list;            /* List to get the length of */
/*
 * Returns the length of the list.  The list must have been
 * already created using lsCreate.
 */
{
    lsDesc *realList = (lsDesc *) list;

    return (realList->length);
}



/*
 * Deleting first and last items of a list
 */

lsStatus lsDelBegin(list, data)
        lsList list;            /* List to delete item from     */
        lsGeneric *data;        /* First item (returned)        */
/*
 * This routine deletes the first item of a list.  The user
 * data associated with the item is returned so the caller
 * may dispose of it.  Returns LS_NOMORE if there is no
 * item to delete.
 */
{
    lsDesc *realList = (lsDesc *) list;
    lsElem *temp;

    if (realList->topPtr == NIL(lsElem)) {
        /* Nothing to delete */
        *data = (lsGeneric) 0;
        return LS_NOMORE;
    } else {
        *data = realList->topPtr->userData;
        temp = realList->topPtr;
        realList->topPtr = realList->topPtr->nextPtr;
        if (temp->nextPtr != NIL(lsElem)) {
            /* There is something after the first item */
            temp->nextPtr->prevPtr = NIL(lsElem);
        } else {
            /* Nothing after it - bottom becomes null as well */
            realList->botPtr = NIL(lsElem);
        }
        free((lsGeneric) temp);
        realList->length -= 1;
    }
    return LS_OK;
}


lsStatus lsDelEnd(list, data)
        lsList list;            /* List to delete item from */
        lsGeneric *data;            /* Last item (returned)     */
/*
 * This routine deletes the last item of a list.  The user
 * data associated with the item is returned so the caller
 * may dispose of it.  Returns LS_NOMORE if there is nothing
 * to delete.
 */
{
    lsDesc *realList = (lsDesc *) list;
    lsElem *temp;

    if (realList->botPtr == NIL(lsElem)) {
        /* Nothing to delete */
        *data = (lsGeneric) 0;
        return LS_NOMORE;
    } else {
        *data = realList->botPtr->userData;
        temp = realList->botPtr;
        realList->botPtr = realList->botPtr->prevPtr;
        if (temp->prevPtr != NIL(lsElem)) {
            /* There is something before the last item */
            temp->prevPtr->nextPtr = NIL(lsElem);
        } else {
            /* Nothing before it - top becomes null as well */
            realList->topPtr = NIL(lsElem);
        }
        free((lsGeneric) temp);
        realList->length -= 1;
    }
    return LS_OK;
}


/*
 * List Generation Routines
 *
 * nowPtr is the element just before the next one to be generated
 */

lsGen lsStart(list)
        lsList list;            /* List to generate items from */
/*
 * This routine defines a generator which is used to step through
 * each item of the list.  It returns a generator handle which should
 * be used when calling lsNext, lsPrev, lsInBefore, lsInAfter, lsDelete,
 * or lsFinish.
 */
{
    lsDesc        *realList = (lsDesc *) list;
    lsGenInternal *newGen;

    newGen = alloc(lsGenInternal);
    newGen->mainList   = realList;
    newGen->beforeSpot = NIL(lsElem);
    newGen->afterSpot  = realList->topPtr;
    return ((lsGen) newGen);
}

lsGen lsEnd(list)
        lsList list;            /* List to generate items from */
/*
 * This routine defines a generator which is used to step through
 * each item of a list.  The generator is initialized to the end 
 * of the list.
 */
{
    lsDesc        *realList = (lsDesc *) list;
    lsGenInternal *newGen;

    newGen = alloc(lsGenInternal);
    newGen->mainList   = realList;
    newGen->beforeSpot = realList->botPtr;
    newGen->afterSpot  = NIL(lsElem);
    return (lsGen) newGen;
}

lsGen lsGenHandle(itemHandle, data, option)
        lsHandle itemHandle;        /* Handle of an item         */
        lsGeneric *data;            /* Data associated with item */
        int option;            /* LS_BEFORE or LS_AFTER     */
/*
 * This routine produces a generator given a handle.  Handles
 * are produced whenever an item is added to a list.  The generator
 * produced by this routine may be used when calling any of 
 * the standard generation routines.  NOTE:  the generator
 * should be freed using lsFinish.  The 'option' parameter
 * determines whether the generator spot is before or after
 * the handle item.
 */
{
    lsElem        *realItem = (lsElem *) itemHandle;
    lsGenInternal *newGen;

    newGen = alloc(lsGenInternal);
    newGen->mainList = realItem->mainList;
    *data = realItem->userData;
    if (option & LS_BEFORE) {
        newGen->beforeSpot = realItem->prevPtr;
        newGen->afterSpot  = realItem;
    } else if (option & LS_AFTER) {
        newGen->beforeSpot = realItem;
        newGen->afterSpot  = realItem->nextPtr;
    } else {
        free((lsGeneric) newGen);
        newGen = (lsGenInternal *) 0;
    }
    return ((lsGen) newGen);
}


lsStatus lsNext(generator, data, itemHandle)
        lsGen generator;        /* Generator handle        */
        lsGeneric *data;            /* User data (return)      */
        lsHandle *itemHandle;        /* Handle to item (return) */
/*
 * Generates the item after the item previously generated by lsNext
 * or lsPrev.   It returns a pointer to the user data structure in 'data'.  
 * 'itemHandle' may be used to get a generation handle without
 * generating through the list to find the item.  If there are no more 
 * elements to generate, the routine returns  LS_NOMORE (normally it 
 * returns LS_OK).  lsNext DOES NOT automatically clean up after all 
 * elements have been generated.  lsFinish must be called explicitly to do this.
 */
{
    register lsGenInternal *realGen = (lsGenInternal *) generator;

    if (realGen->afterSpot == NIL(lsElem)) {
        /* No more stuff to generate */
        *data                       = (lsGeneric) 0;
        if (itemHandle) *itemHandle = (lsHandle) 0;
        return LS_NOMORE;
    } else {
        *data                       = realGen->afterSpot->userData;
        if (itemHandle) *itemHandle = (lsHandle) (realGen->afterSpot);
        /* Move the pointers down one */
        realGen->beforeSpot = realGen->afterSpot;
        realGen->afterSpot  = realGen->afterSpot->nextPtr;
        return LS_OK;
    }
}


lsStatus lsPrev(generator, data, itemHandle)
        lsGen generator;        /* Generator handle        */
        lsGeneric *data;        /* User data (return)      */
        lsHandle *itemHandle;        /* Handle to item (return) */
/*
 * Generates the item before the item previously generated by lsNext
 * or lsPrev.   It returns a pointer to the user data structure in 'data'.  
 * 'itemHandle' may be used to get a generation handle without
 * generating through the list to find the item.  If there are no more 
 * elements to generate, the routine returns  LS_NOMORE (normally it 
 * returns LS_OK).  lsPrev DOES NOT automatically clean up after all 
 * elements have been generated.  lsFinish must be called explicitly to do this.
 */
{
    register lsGenInternal *realGen = (lsGenInternal *) generator;

    if (realGen->beforeSpot == NIL(lsElem)) {
        /* No more stuff to generate */
        *data                       = (lsGeneric) 0;
        if (itemHandle) *itemHandle = (lsHandle) 0;
        return LS_NOMORE;
    } else {
        *data                       = realGen->beforeSpot->userData;
        if (itemHandle) *itemHandle = (lsHandle) (realGen->beforeSpot);
        /* Move the pointers down one */
        realGen->afterSpot  = realGen->beforeSpot;
        realGen->beforeSpot = realGen->beforeSpot->prevPtr;
        return LS_OK;
    }

}

lsStatus lsInBefore(generator, data, itemHandle)
        lsGen generator;        /* Generator handle          */
        lsGeneric data;            /* Arbitrary pointer to data */
        lsHandle *itemHandle;        /* Handle to item (return) */
/*
 * Inserts an element BEFORE the current spot.  The item generated
 * by lsNext will be unchanged;  the inserted item will be generated
 * by lsPrev.  This modifies the list.  'itemHandle' may be used at 
 * a later time to produce a generation handle without generating 
 * through the list.
 */
{
    lsGenInternal *realGen = (lsGenInternal *) generator;
    lsElem        *newElem;

    if (realGen->beforeSpot == NIL(lsElem)) {
        /* Item added to the beginning of the list */
        (void) lsNewBegin((lsList) realGen->mainList, data, itemHandle);
        realGen->beforeSpot = realGen->mainList->topPtr;
        return LS_OK;
    } else if (realGen->afterSpot == NIL(lsElem)) {
        /* Item added to the end of the list */
        (void) lsNewEnd((lsList) realGen->mainList, data, itemHandle);
        realGen->afterSpot = realGen->mainList->botPtr;
        return LS_OK;
    } else {
        /* Item added in the middle of the list */
        newElem = alloc(lsElem);
        newElem->mainList            = realGen->mainList;
        newElem->prevPtr             = realGen->beforeSpot;
        newElem->nextPtr             = realGen->afterSpot;
        newElem->userData            = data;
        realGen->beforeSpot->nextPtr = newElem;
        realGen->afterSpot->prevPtr  = newElem;
        realGen->beforeSpot          = newElem;
        realGen->mainList->length += 1;
        if (itemHandle) *itemHandle = (lsHandle) newElem;
        return LS_OK;
    }
}

lsStatus lsInAfter(generator, data, itemHandle)
        lsGen generator;        /* Generator handle          */
        lsGeneric data;            /* Arbitrary pointer to data */
        lsHandle *itemHandle;        /* Handle to item (return)   */
/*
 * Inserts an element AFTER the current spot.  The next item generated
 * by lsNext will be the new element.  The next  item generated by
 * lsPrev is unchanged.  This modifies the list.  'itemHandle' may
 * be used at a later time to generate a generation handle without
 * searching through the list to find the item.
 */
{
    lsGenInternal *realGen = (lsGenInternal *) generator;
    lsElem        *newElem;

    if (realGen->beforeSpot == NIL(lsElem)) {
        /* Item added to the beginning of the list */
        (void) lsNewBegin((lsList) realGen->mainList, data, itemHandle);
        realGen->beforeSpot = realGen->mainList->topPtr;
        return LS_OK;
    } else if (realGen->afterSpot == NIL(lsElem)) {
        /* Item added to the end of the list */
        (void) lsNewEnd((lsList) realGen->mainList, data, itemHandle);
        realGen->afterSpot = realGen->mainList->botPtr;
        return LS_OK;
    } else {
        /* Item added in the middle of the list */
        newElem = alloc(lsElem);
        newElem->mainList            = realGen->mainList;
        newElem->prevPtr             = realGen->beforeSpot;
        newElem->nextPtr             = realGen->afterSpot;
        newElem->userData            = data;
        realGen->beforeSpot->nextPtr = newElem;
        realGen->afterSpot->prevPtr  = newElem;
        realGen->afterSpot           = newElem;
        realGen->mainList->length += 1;
        if (itemHandle) *itemHandle = (lsHandle) newElem;
        return LS_OK;
    }
}


lsStatus lsDelBefore(generator, data)
        lsGen generator;        /* Generator handle        */
        lsGeneric *data;            /* Deleted item (returned) */
/*
 * Removes the item before the current spot.  The next call to lsPrev
 * will return the item before the deleted item.  The next call to lsNext
 * will be uneffected.  This modifies the list.  The routine returns 
 * LS_BADSTATE if the user tries to call the routine and there is
 * no item before the current spot.  This routine returns the userData
 * of the deleted item so it may be freed (if necessary).
 */
{
    lsGenInternal *realGen = (lsGenInternal *) generator;
    lsElem        *doomedItem;

    if (realGen->beforeSpot == NIL(lsElem)) {
        /* No item to delete */
        *data = (lsGeneric) 0;
        return LS_BADSTATE;
    } else if (realGen->beforeSpot == realGen->mainList->topPtr) {
        /* Delete the first item of the list */
        realGen->beforeSpot = realGen->beforeSpot->prevPtr;
        return lsDelBegin((lsList) realGen->mainList, data);
    } else if (realGen->beforeSpot == realGen->mainList->botPtr) {
        /* Delete the last item of the list */
        realGen->beforeSpot = realGen->beforeSpot->prevPtr;
        return lsDelEnd((lsList) realGen->mainList, data);
    } else {
        /* Normal mid list deletion */
        doomedItem = realGen->beforeSpot;
        doomedItem->prevPtr->nextPtr = doomedItem->nextPtr;
        doomedItem->nextPtr->prevPtr = doomedItem->prevPtr;
        realGen->beforeSpot          = doomedItem->prevPtr;
        realGen->mainList->length -= 1;
        *data = doomedItem->userData;
        free((lsGeneric) doomedItem);
        return LS_OK;
    }
}


lsStatus lsDelAfter(generator, data)
        lsGen generator;        /* Generator handle        */
        lsGeneric *data;            /* Deleted item (returned) */
/*
 * Removes the item after the current spot.  The next call to lsNext
 * will return the item after the deleted item.  The next call to lsPrev
 * will be uneffected.  This modifies the list.  The routine returns 
 * LS_BADSTATE if the user tries to call the routine and there is
 * no item after the current spot.  This routine returns the userData
 * of the deleted item so it may be freed (if necessary).
 */
{
    lsGenInternal *realGen = (lsGenInternal *) generator;
    lsElem        *doomedItem;

    if (realGen->afterSpot == NIL(lsElem)) {
        /* No item to delete */
        *data = (lsGeneric) 0;
        return LS_BADSTATE;
    } else if (realGen->afterSpot == realGen->mainList->topPtr) {
        /* Delete the first item of the list */
        realGen->afterSpot = realGen->afterSpot->nextPtr;
        return lsDelBegin((lsList) realGen->mainList, data);
    } else if (realGen->afterSpot == realGen->mainList->botPtr) {
        /* Delete the last item of the list */
        realGen->afterSpot = realGen->afterSpot->nextPtr;
        return lsDelEnd((lsList) realGen->mainList, data);
    } else {
        /* Normal mid list deletion */
        doomedItem = realGen->afterSpot;
        doomedItem->prevPtr->nextPtr = doomedItem->nextPtr;
        doomedItem->nextPtr->prevPtr = doomedItem->prevPtr;
        realGen->afterSpot           = doomedItem->nextPtr;
        realGen->mainList->length -= 1;
        *data = doomedItem->userData;
        free((lsGeneric) doomedItem);
        return LS_OK;
    }
}



lsStatus lsFinish(generator)
        lsGen generator;        /* Generator handle */
/*
 * Marks the completion of a generation of list items.  This routine should
 * be called after calls to lsNext to free resources used by the
 * generator.  This rule applies even if all items of a list are
 * generated by lsNext.
 */
{
    lsGenInternal *realGen = (lsGenInternal *) generator;

    free((lsGeneric) realGen);
    return (LS_OK);
}


/*
 * Functional list generation
 *
 * An alternate form of generating through items of a list is provided.
 * The routines below generatae through all items of a list in a given
 * direction and call a user provided function for each one.
 */

static lsStatus lsGenForm();

lsStatus lsForeach(list, userFunc, arg)
        lsList list;            /* List to generate through */
        lsStatus (*userFunc)();        /* User provided function   */
        lsGeneric arg;            /* User provided data       */
/*
 * This routine generates all items in `list' from the first item
 * to the last calling `userFunc' for each item.  The function
 * should have the following form:
 *   lsStatus userFunc(data, arg)
 *   lsGeneric data;
 *   lsGeneric arg;
 * `data' will be the user data associated with the item generated.
 * `arg' will be the same pointer provided to lsForeach.  The
 * routine should return LS_OK to continue the generation,  LS_STOP
 * to stop generating items,  and LS_DELETE to delete the item
 * from the list.  If the generation was stopped prematurely,
 * the routine will return LS_STOP.  If the user provided function
 * does not return an appropriate value,  the routine will return
 * LS_BADPARAM.
 */
{
    return lsGenForm(userFunc, arg, lsStart(list), lsNext, lsDelBefore);
}


lsStatus lsBackeach(list, userFunc, arg)
        lsList list;            /* List to generate through */
        lsStatus (*userFunc)();        /* User provided function   */
        lsGeneric arg;            /* User provided data       */
/*
 * This routine is just like lsForeach except it generates
 * all items in `list' from the last item to the first.
 */
{
    return lsGenForm(userFunc, arg, lsEnd(list), lsPrev, lsDelAfter);
}



static lsStatus lsGenForm(userFunc, arg, gen, gen_func, del_func)
        lsStatus (*userFunc)();        /* User provided function         */
        lsGeneric arg;            /* Data to pass to function       */
        lsGen gen;            /* Generator to use               */
        lsStatus (*gen_func)();        /* Generator function to use      */
        lsStatus (*del_func)();        /* Deletion function to use       */
/*
 * This is the function used to implement the two functional
 * generation interfaces to lists.
 */
{
    lsGeneric data;

    while ((*gen_func)(gen, &data, LS_NH) == LS_OK) {
        switch ((*userFunc)(data, arg)) {
            case LS_OK:
                /* Nothing */
                break;
            case LS_STOP: (void) lsFinish(gen);
                return LS_STOP;
            case LS_DELETE: (*del_func)(gen, &data);
                break;
            default: return LS_BADPARAM;
        }
    }
    (void) lsFinish(gen);
    return LS_OK;
}


lsList lsQueryHandle(itemHandle)
        lsHandle itemHandle;        /* Handle of an item  */
/*
 * This routine returns the associated list of the specified
 * handle.  Returns 0 if there were problems.
 */
{
    lsElem *realHandle = (lsElem *) itemHandle;

    if (realHandle) {
        return (lsList) realHandle->mainList;
    } else {
        return (lsList) 0;
    }
}

lsGeneric lsFetchHandle(itemHandle)
        lsHandle itemHandle;
/*
 * This routine returns the user data of the item associated with
 * `itemHandle'.
 */
{
    return ((lsElem *) itemHandle)->userData;
}

lsStatus lsRemoveItem(itemHandle, userData)
        lsHandle itemHandle;        /* Handle of an item */
        lsGeneric *userData;        /* Returned data     */
/*
 * This routine removes the item associated with `handle' from
 * its list and returns the user data associated with the item
 * for reclaimation purposes.  Note this modifies the list
 * that originally contained `item'.
 */
{
    lsElem        *realItem = (lsElem *) itemHandle;
    lsGenInternal gen;

    gen.mainList   = realItem->mainList;
    gen.beforeSpot = realItem->prevPtr;
    gen.afterSpot  = realItem;
    return lsDelAfter((lsGen) &gen, userData);
}


/* List sorting support */
#define TYPE        lsElem
#define SORT        lsSortItems
#define NEXT        nextPtr
#define FIELD        userData

#include "lsort.h"        /* Merge sort by R. Rudell */

lsStatus lsSort(list, compare)
        lsList list;            /* List to sort        */
        int (*compare)();        /* Comparison function */
/*
 * This routine sorts `list' using `compare' as the comparison
 * function between items in the list.  `compare' has the following form:
 *   int compare(item1, item2)
 *   lsGeneric item1, item2;
 * The routine should return -1 if item1 is less than item2, 0 if
 * they are equal,  and 1 if item1 is greater than item2.
 * The routine uses a generic merge sort written by Rick Rudell.
 */
{
    lsDesc *realList = (lsDesc *) list;
    lsElem *idx, *lastElem;

    realList->topPtr = lsSortItems(realList->topPtr, compare,
                                   realList->length);

    /* Forward pointers are correct - fix backward pointers */
    lastElem = (lsElem *) 0;
    for (idx = realList->topPtr; idx != (lsElem *) 0; idx = idx->nextPtr) {
        idx->prevPtr = lastElem;
        lastElem = idx;
    }
    /* lastElem is last item in list */
    realList->botPtr = lastElem;
    return LS_OK;
}



lsStatus lsUniq(list, compare, delFunc)
        lsList list;            /* List to remove duplicates from */
        int (*compare)();        /* Item comparison function       */
        void (*delFunc)();        /* Function to release user data  */
/*
 * This routine takes a sorted list and removes all duplicates
 * from it.  `compare' has the following form:
 *   int compare(item1, item2)
 *   lsGeneric item1, item2;
 * The routine should return -1 if item1 is less than item2, 0 if
 * they are equal,  and 1 if item1 is greater than item2. `delFunc'
 * will be called with a pointer to a user data item for each
 * duplicate destroyed.  `delFunc' can be zero if no clean up
 * is required.
 */
{
    lsGeneric     this_item, last_item;
    lsGenInternal realGen;
    lsDesc        *realList = (lsDesc *) list;

    if (realList->length > 1) {
        last_item = realList->topPtr->userData;

        /* Inline creation of generator */
        realGen.mainList   = realList;
        realGen.beforeSpot = realList->topPtr;
        realGen.afterSpot  = realList->topPtr->nextPtr;

        while (realGen.afterSpot) {
            this_item = realGen.afterSpot->userData;
            if ((*compare)(this_item, last_item) == 0) {
                /* Duplicate -- eliminate */
                (void) lsDelAfter((lsGen) &realGen, &this_item);
                if (delFunc) (*delFunc)(this_item);
            } else {
                /* Move generator forward */
                realGen.beforeSpot = realGen.afterSpot;
                realGen.afterSpot  = realGen.afterSpot->nextPtr;
                last_item = this_item;
            }
        }
    }
    return LS_OK;
}
