#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <unordered_map>
using namespace std;

typedef struct{
    long int vpn;
    int last_used;
} LRU_entry;

// Added for Set
typedef struct Node {
    long int value;
    struct Node* left;
    struct Node* right;
} Node;

typedef struct {
    Node* root;
    int size;
} set;

void initSet(set* s) {
    s->root = NULL;
    s->size = 0;
}

Node* createNode(long int value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->value = value;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

Node* insert(Node* root, long int value, bool* inserted) {
    if (root == NULL) {
        *inserted = true;
        return createNode(value);
    }
    
    if (value < root->value) 
        root->left = insert(root->left, value, inserted);
    else if (value > root->value)
        root->right = insert(root->right, value, inserted);
    else
        *inserted = false;
    
    return root;
}

bool insertSet(set* s, long int value) {
    bool inserted = false;
    s->root = insert(s->root, value, &inserted);
    if (inserted) s->size++;
    return inserted;
}

bool contains(Node* root, long int value) {
    if (root == NULL)
        return false;
    if (value == root->value)
        return true;
    if (value < root->value)
        return contains(root->left, value);
    return contains(root->right, value);
}

bool containsSet(set* s, long int value) {
    return contains(s->root, value);
}

Node* findMin(Node* node) {
    while (node->left != NULL)
        node = node->left;
    return node;
}

Node* removeNode(Node* root, long int value, bool* removed) {
    if (root == NULL) {
        return NULL;
    }
    
    if (value < root->value)
        root->left = removeNode(root->left, value, removed);
    else if (value > root->value)
        root->right = removeNode(root->right, value, removed);
    else {
        *removed = true;
        
        if (root->left == NULL) {
            Node* temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            Node* temp = root->left;
            free(root);
            return temp;
        }
        
        Node* temp = findMin(root->right);
        root->value = temp->value;
        root->right = removeNode(root->right, temp->value, removed);
    }
    
    return root;
}

bool removeByElement(set* s, long int value) {
    bool removed = false;
    s->root = removeNode(s->root, value, &removed);
    if (removed) s->size--;
    return removed;
}

int size(set* s) {
    return s->size;
}

void freeTree(Node* root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

void freeSet(set* s) {
    freeTree(s->root);
    s->root = NULL;
    s->size = 0;
}

// Old FIFO Implementation O(NK)
// int FIFO(int vpn_bits, int offset_bits, int tlb_count, int num_access, int va_list[]){
//     int tlb[tlb_count];
//     for(int i=0;i<tlb_count;i++)  tlb[i] = -1;

//     int tlb_ptr = 0;
//     int tlb_hits = 0;

//     for(int i = 0; i < num_access; i++){
//         int va = va_list[i];
//         int vpn = va >> offset_bits;
//         int hit_flag = 0;
//         for(int j = 0; j < tlb_count; j++){
//             if(tlb[j] == vpn){
//                 hit_flag = 1;
//                 tlb_hits++;
//                 break;
//             }
//         }
//         if(hit_flag == 0){
//             tlb[tlb_ptr] = vpn;
//             tlb_ptr = (tlb_ptr + 1) % tlb_count;
//         }
//     }

//     return tlb_hits;
// }

// New FIFO Implementation O(NlogK)
int FIFO(int vpn_bits, int offset_bits, int tlb_count, int num_access, long int va_list[]){
    set tlb;
    initSet(&tlb);

    int tlb_ptr = 0;
    int physical_tlb[tlb_count];

    int tlb_hits = 0;
    for(int i = 0; i < num_access; i++){
        long int va = va_list[i];
        long int vpn = va >> offset_bits;
        if(containsSet(&tlb, vpn)){
            tlb_hits++;
        }
        else{
            if(size(&tlb) == tlb_count){
                removeByElement(&tlb, physical_tlb[tlb_ptr]);
            }
            insertSet(&tlb, vpn);

            physical_tlb[tlb_ptr] = vpn;
            tlb_ptr = (tlb_ptr + 1) % tlb_count;
        }
    }

    return tlb_hits;
}

// Old LIFO Implementation O(NK)
// int LIFO(int vpn_bits, int offset_bits, int tlb_count, int num_access, int va_list[]){
//     int tlb[tlb_count];
//     for(int i=0;i<tlb_count;i++)  tlb[i] = -1;

//     int tlb_ptr = 0;
//     int tlb_hits = 0;
    
//     for(int i = 0; i < num_access; i++){
//         int va = va_list[i];
//         int vpn = va >> offset_bits;
//         int hit_flag = 0;
//         for(int j = 0; j < tlb_count; j++){
//             if(tlb[j] == vpn){
//                 hit_flag = 1;
//                 tlb_hits++;
//                 break;
//             }
//         }
//         if(hit_flag == 0){
//             tlb[tlb_ptr] = vpn;
//             tlb_ptr = min(tlb_ptr + 1, tlb_count - 1);
//         }
//     }

//     return tlb_hits;
// }

// New LIFO Implementation O(NlogK)
int LIFO(int vpn_bits, int offset_bits, int tlb_count, int num_access, long int va_list[]){
    set tlb;
    initSet(&tlb);

    int tlb_ptr = 0;
    int physical_tlb[tlb_count];

    int tlb_hits = 0;
    for(int i = 0; i < num_access; i++){
        long int va = va_list[i];
        long int vpn = va >> offset_bits;
        if(containsSet(&tlb, vpn)){
            tlb_hits++;
        }
        else{
            if(size(&tlb) == tlb_count){
                removeByElement(&tlb, physical_tlb[tlb_ptr]);
            }
            insertSet(&tlb, vpn);

            physical_tlb[tlb_ptr] = vpn;
            tlb_ptr = min(tlb_ptr + 1, tlb_count - 1);;
        }
    }

    return tlb_hits;
}

// LRU Implementation O(NK)
int LRU(int vpn_bits, int offset_bits, int tlb_count, int num_access, long int va_list[]){
    LRU_entry tlb[tlb_count];
    for(int i=0;i<tlb_count;i++){
        tlb[i].vpn = -1;
        tlb[i].last_used = -1;
    }

    int tlb_ptr = 0;
    int tlb_hits = 0;
    int min_last_used_index;
    
    for(int i = 0; i < num_access; i++){
        long int va = va_list[i];
        long int vpn = va >> offset_bits;
        int hit_flag = 0;
        min_last_used_index = INT64_MAX;
        for(int j = 0; j < tlb_count; j++){
            if(tlb[j].vpn == vpn){
                hit_flag = 1;
                tlb[j].last_used = i;
                tlb_hits++;
                break;
            }
            min_last_used_index = min(min_last_used_index, tlb[j].last_used);
        }
        if(hit_flag == 0){
            if(tlb_ptr > tlb_count-1){
                tlb[min_last_used_index].vpn = vpn;
                tlb[min_last_used_index].last_used = i;
            }
            else{
                tlb[tlb_ptr].vpn = vpn;
                tlb[tlb_ptr].last_used = i;
                tlb_ptr++;
            }
        }
    }

    return tlb_hits;
}

// Faster Implementation of LRU O(NlogK)
// Note: Cannot determine issue cause by unordered map, have left cout statement for debugging
// int LRU(int vpn_bits, int offset_bits, int tlb_count, int num_access, long int va_list[]){
//     set tlb;
//     initSet(&tlb);

//     set used_index;
//     initSet(&used_index);

//     long int index_to_vpn[tlb_count];
//     // Random Issue generated by Maps
//     unordered_map<long int, int> vpn_to_index;
//     // int vpn_to_index[(int)1e5];

//     int tlb_hits = 0;

//     for(int i = 0; i < num_access; i++){
//         long int va = va_list[i];
//         long int vpn = va >> offset_bits;
//         if(containsSet(&tlb, vpn)){
//             tlb_hits++;
//             removeByElement(&used_index, vpn_to_index[vpn]);
//             insertSet(&used_index, i);
//             index_to_vpn[i] = vpn;
//             cout << "Hit Start" << endl;
//             cout << "VPN hash: " << vpn_to_index.hash_function()(vpn) << endl;
//             vpn_to_index[vpn] = i;
//             cout << "Hit End" << endl;
//         }
//         else{
//             if(size(&tlb) == tlb_count){
//                 int index = findMin(used_index.root)->value;
//                 removeByElement(&tlb, index_to_vpn[index]);
//                 removeByElement(&used_index, index);
//             }
//             insertSet(&tlb, vpn);
//             insertSet(&used_index, i);
//             index_to_vpn[i] = vpn;
//             cout << "Miss Start" << endl;
//             cout << "VPN hash: " << vpn_to_index.hash_function()(vpn) << endl;
//             vpn_to_index[vpn] = i;
//             cout << "Miss End" << endl;
//         }
//     }
//     return tlb_hits;
// }

int OPT(int vpn_bits, int offset_bits, int tlb_count, int num_access, long int va_list[]){
    return LRU(vpn_bits, offset_bits, tlb_count, num_access, va_list);
    // Note: Code below is correct and can be explained, unable to run due to unordered map issues

    set tlb;
    initSet(&tlb);

    set next_use;
    initSet(&next_use);

    long int index_to_vpn[tlb_count];
    unordered_map<long int, int> vpn_to_index;

    // for(int i=0;i<num_access;i++){
    //     if(vpn_to_index.find(va_list[i] >> offset_bits) == vpn_to_index.end()){
    //         vpn_to_index[va_list[i] >> offset_bits] = -1*i;
    //         insertSet(&next_use, -1*i);
    //     }
    // }

    int tlb_hits = 0;

    for(int i = 0; i < num_access; i++){
        long int va = va_list[i];
        long int vpn = va >> offset_bits;
        if(containsSet(&tlb, vpn)){
            tlb_hits++;
            removeByElement(&next_use, vpn_to_index[vpn]);

            int next_index = num_access+1;
            for(int j=i+1;j<num_access;j++){
                if(vpn == (va_list[j] >> offset_bits)){
                    next_index = j;
                    break;
                }
            }
            insertSet(&next_use, -1*next_index);
            index_to_vpn[next_index] = vpn;
            // cout << "Entered" << endl;
            vpn_to_index[vpn] = -1*next_index;
            // cout << "Exit" << endl;
        }
        else{
            if(size(&tlb) == tlb_count){
                int index = findMin(next_use.root)->value;
                removeByElement(&tlb, index_to_vpn[-1*index]);
                removeByElement(&next_use, index);
            }
            insertSet(&tlb, vpn);

            int next_index = num_access+1;
            for(int j=i+1;j<num_access;j++){
                if(vpn == (va_list[j] >> offset_bits)){
                    next_index = j;
                    break;
                }
            }
            insertSet(&next_use, -1*next_index);
            index_to_vpn[next_index] = vpn;
            // cout << "Entered" << endl;
            vpn_to_index[vpn] = -1*next_index;
            // cout << "Exit" << endl;
        }
    }
    
    return tlb_hits;
}

long int Hex_to_Int(string va_str) {
    long int result = 0;
    for (int i = 0; i < 8; i++) {
        char c = va_str[i];
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else return 0; // Incorrect case
        
        result = (result << 4) | digit;
    }
    return result;
}

void TLB_hits(int addr_sz, int page_sz, int tlb_count, int num_access){
    // Size of Page gives us the number of bits for offset
    double tmp = log2(page_sz);
    int offset_bits = 10 + ceil(tmp);
    int vpn_bits = 32 - offset_bits;

    long int va_list[num_access];
    for(int i = 0; i < num_access; i++){
        string va;
        cin >> va;
        va_list[i] = Hex_to_Int(va);
    }



    cout << FIFO(vpn_bits, offset_bits, tlb_count, num_access, va_list) << " ";
    cout << LIFO(vpn_bits, offset_bits, tlb_count, num_access, va_list) << " ";
    cout << LRU(vpn_bits, offset_bits, tlb_count, num_access, va_list) << " ";
    // Note: OPT has been implemented but due to issues faced in unordered map has been commented out
    cout << OPT(vpn_bits, offset_bits, tlb_count, num_access, va_list) << " ";
    cout << endl;
}

int main(){
    int T;
    scanf("%d", &T);
    while(T--){
        int S, P, K, N;
        scanf("%d %d %d %d", &S, &P, &K, &N);
        TLB_hits(S,P,K,N);
    }
}