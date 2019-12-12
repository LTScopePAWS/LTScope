/******************************************************************************
 *  File:         dci_prune.c
 *
 *  Description:  DCI pruning functions 
 *		  Prune the random dci messages generated from one location
 *
 *  Reference:    Yaxiong's Modifications
 *****************************************************************************/
#include "srslte/phy/ue/ue_dl.h"
#include "srslte/phy/ue/rnti_prune.h"
#include "srslte/phy/ue/lte_scope.h"
#include "srslte/phy/phch/dci.h"
#include "srslte/phy/common/phy_common.h"


/*******************************************************
*		Printing functions 
*******************************************************/
static void display_format(srslte_dci_format_t format){
    switch(format){
        case SRSLTE_DCI_FORMAT0:  printf("FORMAT0\n");  break;
        case SRSLTE_DCI_FORMAT1:  printf("FORMAT1\n");  break;
        case SRSLTE_DCI_FORMAT1A: printf("FORMAT1A\n"); break;
        case SRSLTE_DCI_FORMAT1C: printf("FORMAT1C\n"); break;
        case SRSLTE_DCI_FORMAT1B: printf("FORMAT1B\n"); break;
        case SRSLTE_DCI_FORMAT1D: printf("FORMAT1D\n"); break;
        case SRSLTE_DCI_FORMAT2:  printf("FORMAT2\n");  break;
        case SRSLTE_DCI_FORMAT2A: printf("FORMAT2A\n"); break;
        case SRSLTE_DCI_FORMAT2B: printf("FORMAT2B\n"); break;
        default: printf("Invalid format!\n");
    }
}
void dci_msg_display(srslte_dci_msg_paws* dci_msg_yx){
    printf("%d\t%d\t%d\t%d\t",dci_msg_yx->tti, dci_msg_yx->rnti, dci_msg_yx->downlink, dci_msg_yx->nof_prb);
    printf("%d\t%d\t",dci_msg_yx->mcs_idx_tb1, dci_msg_yx->mcs_idx_tb2);
    printf("%3.1f\t",dci_msg_yx->decode_prob);
    printf("L:%d\tn:%d\t",dci_msg_yx->L,dci_msg_yx->ncce);
    printf("%d\t%d\t%d\t",dci_msg_yx->max_freq_ue,dci_msg_yx->max_freq_ue_cnt, dci_msg_yx->nof_active_ue);
    printf("%d\t%d\t",dci_msg_yx->active, dci_msg_yx->my_cnt);
    printf("%d\t%d\t",dci_msg_yx->tbs_tb1, dci_msg_yx->tbs_tb2);
    printf("%d\t%d\t",dci_msg_yx->tbs_hm_tb1, dci_msg_yx->tbs_hm_tb2);
    display_format(dci_msg_yx->format);
}

void dci_msg_list_display(srslte_dci_msg_paws* dci_msg_yx, int nof_msg){
    for(int i=0;i<nof_msg;i++){
	dci_msg_display(&dci_msg_yx[i]);
    }
}

void record_dci_msg_single(srslte_dci_msg_paws* dci_msg_yx, FILE* FD_DCI){
    fprintf(FD_DCI, "%d\t%d\t%d\t",dci_msg_yx->tti, dci_msg_yx->rnti, dci_msg_yx->nof_prb);
    fprintf(FD_DCI, "%d\t%d\t",dci_msg_yx->mcs_idx_tb1, dci_msg_yx->mcs_idx_tb2);
    fprintf(FD_DCI, "%d\t%d\t",dci_msg_yx->tbs_tb1, dci_msg_yx->tbs_tb2);
    fprintf(FD_DCI, "%d\t%d\t",dci_msg_yx->tbs_hm_tb1, dci_msg_yx->tbs_hm_tb2);
    fprintf(FD_DCI, "%3.1f\t",dci_msg_yx->decode_prob);
    fprintf(FD_DCI, "%d\t%d\t",dci_msg_yx->L,dci_msg_yx->ncce);
    fprintf(FD_DCI, "%d\t%d\t%d\t",dci_msg_yx->max_freq_ue,dci_msg_yx->nof_active_ue, dci_msg_yx->active);
    fprintf(FD_DCI, "%d\n", (int)dci_msg_yx->format);
}

void record_dci_msg_log(srslte_dci_msg_paws* dci_msg_ret, int nof_msg, FILE* FD_DCI)
{
    srslte_dci_msg_paws* dci_msg_yx;
    for(int i=0;i<nof_msg;i++){
	dci_msg_yx = &dci_msg_ret[i];
	record_dci_msg_single(dci_msg_yx, FD_DCI);	    
    }
    return;
}

/*******************************************************/
//	     rnti pruning in one location 	 
/*******************************************************/

uint8_t srslte_check_rnti_inqueue_uedl(srslte_active_ue_list_t* q, uint16_t rnti){
    return q->active_ue_list[rnti];
}

// Count how many formats pass the threlshold check
int count_active_dci_one_location(int* ret_vec){
    int count = 0;
    for (int i=0; i<NOF_UE_ALL_FORMATS;i++){
        if (ret_vec[i] == 1){
            count += 1;
        }
    }
    return count;
}

// Count how many formats are decoded with very high confidence
int count_high_confidence_rnti(srslte_dci_msg_paws* dci_msg_list, int* index){
    int count = 0;
    int idx = 0;

    //printf("decoded prob: -- ");
    for (int i=0; i<NOF_UE_ALL_FORMATS;i++){
	//printf("%5.1f\t", dci_msg_list[i].decode_prob); 
        if (dci_msg_list[i].decode_prob >= HIGH_PROB_THR){
            count += 1;
            idx = i;
        }
    }
    //printf(" NO: %d\n",count);

    if(count == 1){
        *index = idx;
    }
    return count;
}
// High frequency RNTI matches with Max frequency RNTI
int match_high_confidence_freq_rnti(srslte_active_ue_list_t* q, srslte_dci_msg_paws* dci_msg_list){
    int index = -1;
    for (int i=1; i<NOF_UE_ALL_FORMATS;i++){
        if ( (dci_msg_list[i].decode_prob >= HIGH_PROB_THR) && 
		(dci_msg_list[i].rnti == q->max_freq_ue)){
	    index = i;
        }
    }
    return index;
}

// High frequency RNTI matches with active UE list
int match_max_freq_rnti_with_list(srslte_active_ue_list_t* q, srslte_dci_msg_paws* dci_msg_list, int* index){
    int idx = -1;
    int count = 0;
    for (int i=1; i<NOF_UE_ALL_FORMATS;i++){
        if ( (dci_msg_list[i].decode_prob >= HIGH_PROB_THR) && 
		(q->active_ue_list[dci_msg_list[i].rnti]) ){
	    idx = i;
	    count++;
        }
    }
    if(count == 1){
	*index = idx;
    }
    return count;
}

// count how many rnti is decoded within the active UE list  
int count_rnti_in_list(srslte_active_ue_list_t* q,
                        srslte_dci_msg_paws* dci_msg_list,
			int* index)
{
    int count = 0;
    int idx =0;

    for (int i=1;i<NOF_UE_ALL_FORMATS;i++){
        if(srslte_check_rnti_inqueue_uedl(q, dci_msg_list[i].rnti) == 1){
            count++;
	    idx = i;
        }
    }
    if (count == 1){
	*index = idx;
    }
    return count;
}

int count_rnti_in_active_ue_list(uint8_t* active_ue_list,
                        srslte_dci_msg_paws* dci_msg_list,
			int* index)
{
    int count = 0;
    int idx =0;

    for (int i=1;i<NOF_UE_ALL_FORMATS;i++){
        if(active_ue_list[dci_msg_list[i].rnti] == 1){
            count++;
	    idx = i;
        }
    }
    if (count == 1){
	*index = idx;
    }
    return count;
}


void display_all_msg(srslte_dci_msg_paws* dci_msg_list){
    for (int i=1;i<NOF_UE_ALL_FORMATS;i++){
	printf("%3.2f\t",dci_msg_list[i].decode_prob); 
    }
    printf("\n");
}

/* Prune the DCI messages decoded from the same location but with different formats*/

int dci_msg_location_pruning(srslte_active_ue_list_t* q,
                            srslte_dci_msg_paws* dci_msg_list,
                            srslte_dci_msg_paws* dci_decode_ret,
                            uint32_t tti)
{
    int count;
    int index = 0, idx_tmp;
    count = count_high_confidence_rnti(dci_msg_list, &index);
    // If only one format has rnti with prob larger then threshold
    if(count == 1){
        //printf("Only 1 UE has high prob!\n");
        if ( (index == 6) && (dci_msg_list[5].decode_prob >90) ){
            // we prefer format 2 instead of 2A
            index = 5;
        }
        memcpy(dci_decode_ret, &dci_msg_list[index], sizeof(srslte_dci_msg_paws));
        //dci_msg_display(&dci_msg_list[index]);
        //display_all_msg(dci_msg_list);
        return 1;
    }else if(count >1){
        // We have 2 or more high-prob decoding results
	//printf("MORE than 1 high-prob!\n");
        // if the high-prob rnti matches with the max-freq rnti
        idx_tmp = match_high_confidence_freq_rnti(q, dci_msg_list);
        if(idx_tmp >0){
            memcpy(dci_decode_ret, &dci_msg_list[idx_tmp], sizeof(srslte_dci_msg_paws));
            //printf("Two or more UEs have high prob! -- Match with max freq rnti\n");
            //dci_msg_display(q, &dci_msg_list[idx_tmp]);
            return 1;
        }
        // if the high-prob rnti matches is in the list
        int cnt = match_max_freq_rnti_with_list(q, dci_msg_list, &index);
        if(cnt == 1){
            memcpy(dci_decode_ret, &dci_msg_list[index], sizeof(srslte_dci_msg_paws));
            //printf("Two or more UEs have high prob! -- Match with rnti list\n");
            //dci_msg_display(q, &dci_msg_list[index]);
            return 1;
        }
        // it is format 2 or 1A with high probability
        if(dci_msg_list[5].decode_prob > HIGH_PROB_THR){
            // format 2
            memcpy(dci_decode_ret, &dci_msg_list[5], sizeof(srslte_dci_msg_paws));
            //printf("Two or more UEs have high prob! -- format 2\n");
            //dci_msg_display(q, &dci_msg_list[5]);
            return 1;
        }else if(dci_msg_list[2].decode_prob > HIGH_PROB_THR){
            // format 1A
            memcpy(dci_decode_ret, &dci_msg_list[2], sizeof(srslte_dci_msg_paws));
            //printf("Two or more UEs have high prob! -- format 1A\n");
            //dci_msg_display(q, &dci_msg_list[2]);
            return 1;
        }
    }

    int inlist_count = count_rnti_in_list(q, dci_msg_list,&index);
    if (inlist_count == 0){
        return 0;
    }else if(inlist_count == 1){
        //printf("Only 1 UE in list \n");
        memcpy(dci_decode_ret, &dci_msg_list[index], sizeof(srslte_dci_msg_paws));
        //dci_msg_display(q, &dci_msg_list[index]);
        return 1;
    }else{
        printf("We found %d in-list rnti!\n",inlist_count);
        return 0;
    }
    return 0;
}

/*******************************************************/
//     clear the dci location that has been decoded	 
/*******************************************************/
/* Check whether two range has overlap or not*/
static int check_overlapping(int this_ncce_min, int this_ncce_max, int test_ncce_min, int test_ncce_max)
{
    if( (test_ncce_min > this_ncce_max) || (test_ncce_max < this_ncce_min)){
        return 0;
    }else{
        return 1;
    }
}

/* Mark the CCE locations with all aggregation levels as checked, 
* if any cce has been included as part of the DCI message that has 
* been correctly identified as successfully decoded */
void check_decoded_locations(srslte_dci_location_paws_t* locations,
                             uint32_t nof_locations,
                             int this_loc)
{
    int this_ncce_min,this_ncce_max,test_ncce_min,test_ncce_max;
    int check_ret;
    this_ncce_min = locations[this_loc].ncce;
    this_ncce_max = locations[this_loc].ncce + (1 << locations[this_loc].L);
    for(int i=0;i<nof_locations;i++){
        test_ncce_min = locations[i].ncce;
        test_ncce_max = locations[i].ncce + (1 << locations[this_loc].L);
        check_ret = check_overlapping(this_ncce_min, this_ncce_max, test_ncce_min, test_ncce_max);
        if(check_ret == 1){
            locations[i].checked = 1;
        }
    }
}

/*******************************************************/
//     prune the decoded rnti within one subframe 	 
/*******************************************************/

/* Calculate the total number of allocated prb within 1-subframe */
int nof_prb_one_subframe(srslte_dci_msg_paws* dci_ret, int msg_cnt)
{
    int nof_prb = 0; 
    for(int i=0;i<msg_cnt;i++){
	nof_prb += dci_ret[i].nof_prb;
    }
    //printf("Total prb within one subframe: %d \n",nof_prb);
    return nof_prb;
}

/* Divide the dci message list into two sub-list 
* 1 is the list of UEs that are active
* 2 is the list of UEs that are not active UE*/ 
static int extract_intlist_ue(srslte_active_ue_list_t* q,
			srslte_dci_msg_paws*  dci_ret_input, 
			srslte_dci_msg_paws*  dci_inlist,
			srslte_dci_msg_paws*  dci_not_inlist,
			int msg_cnt_input,
			int* nof_prb)
{
    int inlist_count = 0;
    int outof_list_count = 0;
    int prb_sum = 0;

    for(int i=0;i<msg_cnt_input;i++){	
	if(srslte_check_rnti_inqueue_uedl(q, dci_ret_input[i].rnti) == 1){
	    // ue is in the active UE list 
	    dci_inlist[inlist_count] = dci_ret_input[i]; 
	    prb_sum += dci_ret_input[i].nof_prb;
	    inlist_count++;
	}else{
	    dci_not_inlist[outof_list_count] = dci_ret_input[i]; 
	    outof_list_count++;
	}
    }
    *nof_prb = prb_sum;
    return inlist_count;
}

// Definition of the function
static int calculate_combine_sum(srslte_dci_msg_paws* arr, int data[], int* idx_vec, int start, int end, 
			    int index, int r, int NOF_PRB, int MAX_PRB);

/* Traversal all possible combination of dci messages and find the combination that 
*  the total number of decoded control messages equals to the total cell PRBs*/
static int dci_combination_sum(srslte_dci_msg_paws* arr, int* idx_vec, int n, int NOF_PRB, int MAX_PRB){
    int data[n];
    for(int r=1;r<=n;r++){
	if(calculate_combine_sum(arr, data, idx_vec, 0, n-1, 0, r, NOF_PRB, MAX_PRB) == 1){
	    //for(int i=0;i<r;i++){
	    //    printf("%d|%d ", idx_vec[i], data[i]);
	    //}
	    //printf("\n");  
	    return r;
	}
    }
    return 0;
}
/* Implementation of the calculate and sum function */
int calculate_combine_sum(srslte_dci_msg_paws* arr, int data[], int* idx_vec, int start, int end, 
			    int index, int r, int NOF_PRB, int MAX_PRB)
{
    int sum_ret;
    if(index == r)
    {
	int count = NOF_PRB;
	for(int j=0;j<r;j++){
	    count += data[j]; 
	}   
	if(count == MAX_PRB){
	    return 1;
	}else{
	    //printf("nof_comb:%d total PRB:%d\n", nof_comb, count);	
	    return 0;
	}
    }
    for(int i=start;i<=end;i++){
	data[index] = arr[i].nof_prb;
	idx_vec[index] = i;
	sum_ret = calculate_combine_sum(arr, data, idx_vec, i+1, end, index+1, r, NOF_PRB, MAX_PRB);
	if(sum_ret == 1){
	    return 1;
	}
    }
    return 0;
}

/* Copy the dci messages in the input message array 
* to the output message array, according to the index 
* contained inisde the idx_vec array and the number of messages*/

void copy_to_output_dci(srslte_dci_msg_paws*  dci_ret_input,
			srslte_dci_msg_paws*  dci_ret_output,
			int* idx_vec,
			int  dci_num)
{
    for(int i=0;i<dci_num;i++){
	memcpy(&dci_ret_output[i], &dci_ret_input[idx_vec[i]], sizeof(srslte_dci_msg_paws));
    }
    
}

/* Find the dci message in the list that has maximum number 
*  of allocated PRBs. Skip the message if it has been checked
*/
int max_nof_prb(srslte_dci_msg_paws*  dci_msg_vec,
		int* checked,
		int msg_cnt,
		int* index)
{
    int max_nof_prb = 0;
    int idx = 0;
    for(int i=0;i<msg_cnt;i++){
	if(checked[i] == 1) continue;
	if( dci_msg_vec[i].nof_prb > max_nof_prb){
	    max_nof_prb = dci_msg_vec[i].nof_prb;  
	    idx = i;
	}
    }
    *index = idx;
    return  max_nof_prb;
}

/* Handle the decoded dci messages that belong to UE 
* who is not in the active UE list but with total number 
* of allocated PRBs larger then the total cell PRB
*/
int handle_not_inlist_ue(srslte_dci_msg_paws* dci_ret_output,
			    int nof_output_msg,
			  srslte_dci_msg_paws* dci_not_inlist,
			    int nof_not_inlist,
			    int nof_prb, int MAX_PRB)
{
    int checked[nof_not_inlist];
    int index = 0;
    int max_prb;
    for(int i=0;i<nof_not_inlist;i++){
	checked[i] = 0;
    }
    
    for(int i=0;i<nof_not_inlist;i++){
	max_prb = max_nof_prb(dci_not_inlist, checked, nof_not_inlist, &index);
	checked[index] = 1;
	if(nof_prb + max_prb < MAX_PRB){
	    nof_prb += max_prb;
	    memcpy(&dci_ret_output[nof_output_msg], &dci_not_inlist[index], sizeof(srslte_dci_msg_paws));
	    //printf("%d ",dci_not_inlist[index].nof_prb);
	    nof_output_msg++; 
	} 
    }
    //printf("\n");
    return nof_output_msg;
}
// Find the dci message with the smallest allocated PRBs
int min_nof_prb(srslte_dci_msg_paws*  dci_msg_vec,
		int msg_cnt,
		int MAX_PRB)
{
    int min_nof_prb = MAX_PRB;
    for(int i=0;i<msg_cnt;i++){
	if( dci_msg_vec[i].nof_prb < min_nof_prb){
	    min_nof_prb = dci_msg_vec[i].nof_prb;  
	}
    }
    return  min_nof_prb;
}

/* Prune the messages decoded from one subframe 
*  we should keep the number of allocated PRBs smaller than the 
*  total cell PRB number */
int dci_subframe_pruning(srslte_active_ue_list_t* q,
			srslte_dci_msg_paws*  dci_ret_input, 
			srslte_dci_msg_paws*  dci_ret_output,
			int max_prb,
			int msg_cnt_input)
{
    int dci_num;
    int nof_prb = 0;
    int nof_inlist_ue = 0;
    int nof_not_inlist_ue = 0;
    int CELL_MAX_PRB  = max_prb; 
    srslte_dci_msg_paws* dci_inlist;
    srslte_dci_msg_paws* dci_not_inlist;

    // UE that is active or not active 
    dci_inlist	    = (srslte_dci_msg_paws*)malloc(msg_cnt_input * sizeof(srslte_dci_msg_paws));
    dci_not_inlist  = (srslte_dci_msg_paws*)malloc(msg_cnt_input * sizeof(srslte_dci_msg_paws));

    nof_inlist_ue	= extract_intlist_ue(q, dci_ret_input, dci_inlist, dci_not_inlist, msg_cnt_input, &nof_prb); 
    nof_not_inlist_ue	= msg_cnt_input - nof_inlist_ue;

    if(nof_inlist_ue == 0){
	// if there is no active UE
	int idx_vec[nof_not_inlist_ue];
	int dci_num = dci_combination_sum(dci_not_inlist, idx_vec, nof_not_inlist_ue,  0, CELL_MAX_PRB);
	if(dci_num >0){
	    // if there is any combination of dci messages whose total allocated PRB match the cell PRB
	    copy_to_output_dci(dci_not_inlist, dci_ret_output, idx_vec, dci_num); 
	    //printf("No UE in list. Match with MAX_PRB\n");
	    //dci_msg_list_display(q, dci_ret_output, dci_num);
	    free(dci_inlist);free(dci_not_inlist);
	    return dci_num;  
	}
	// If such a combination cannot be found 
	//printf("No UE in list. Doesn't match with MAX_PRB\n");
	int total_msg_output = handle_not_inlist_ue(dci_ret_output, 0, 
						    dci_not_inlist, nof_not_inlist_ue, 0, CELL_MAX_PRB);
	free(dci_inlist);free(dci_not_inlist);
	return total_msg_output;
    }else{ 
	int idx_vec[nof_not_inlist_ue];
	dci_num = dci_combination_sum(dci_not_inlist, idx_vec, nof_not_inlist_ue,  0, CELL_MAX_PRB);
	if(dci_num >0){
	    // if there is any combination of dci messages of inactive UEs
	    //  whose total allocated PRB match the cell PRB
	    copy_to_output_dci(dci_not_inlist, dci_ret_output, idx_vec, dci_num); 
	    //printf("UE in list. But UE not in list match with MAX_PRB\n");
	    //dci_msg_list_display(q, dci_ret_output, dci_num);
	    free(dci_inlist);free(dci_not_inlist);
	    return dci_num;  
	}

	// The dci messages of active UE is highly reliable and thus we always keep it
	memcpy(dci_ret_output, dci_inlist, nof_inlist_ue*sizeof(srslte_dci_msg_paws));
	
	if( (nof_prb >= CELL_MAX_PRB) || 
		    (nof_prb + min_nof_prb(dci_not_inlist, nof_not_inlist_ue, CELL_MAX_PRB)) > CELL_MAX_PRB ){
	    // If the total number of allocated PRB of active UE equals to the total PRB
	    //printf("UE in list. Larger than MAX_PRB\n");
	    free(dci_inlist);free(dci_not_inlist);
	    return nof_inlist_ue;
	}
		
	dci_num = dci_combination_sum(dci_not_inlist, idx_vec, nof_not_inlist_ue,  nof_prb, CELL_MAX_PRB);
	if(dci_num >0){
	    // if there is any combination of dci messages of a mix of active and inactive UEs
	    // whose total allocated PRB match the cell PRB
	    copy_to_output_dci(dci_not_inlist, &dci_ret_output[nof_inlist_ue], idx_vec, dci_num); 
	    //printf("UE in list. Match with MAX_PRB\n");
	    //dci_msg_list_display(q, dci_ret_output, nof_inlist_ue+dci_num);
	    free(dci_inlist);free(dci_not_inlist);
	    return nof_inlist_ue+dci_num;  
	}
	// If such a combination cannot be found 
	//printf("UE in list. Doesn't match with MAX_PRB\n");
	int total_msg_output = handle_not_inlist_ue(dci_ret_output, nof_inlist_ue, 
						    dci_not_inlist, nof_not_inlist_ue, nof_prb, CELL_MAX_PRB);
	free(dci_inlist);free(dci_not_inlist);
	return total_msg_output;
    }
    free(dci_inlist);free(dci_not_inlist);
    return 0;
}
