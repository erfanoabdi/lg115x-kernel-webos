#!/bin/sh
if [ $# -eq 0 ]
then
	echo "Usage: sh $0 TARGET_DIR "
	exit 1
fi

TARGET_DIR=$1
#TARGET_DIR=./capture_hdmi_1080i
#TARGET_DIR=./capture_test
RESULT_FILE=result.txt

if [ -f ${RESULT_FILE} ]; then
	rm ${RESULT_FILE}
fi

for k in ${TARGET_DIR}/*.txt;do
echo $k
echo $k >> ${RESULT_FILE}


IS_2D=`expr match "$k" "${TARGET_DIR}/CAP_2D.*$"`
IS_SS=`expr match "$k" "${TARGET_DIR}/CAP_SS.*$"`
IS_TB=`expr match "$k" "${TARGET_DIR}/CAP_TB.*$"`


sed '

s/.*MESSAGE_B= //g
s/\[.*\]*//g
/^$/d
#/MESSAGE.\_.A$/d

' $k > ${k}.0

# for org_fmt parameter 
if [ "$IS_2D" -gt 0 ]; then
awk '{print 0, $0 }' ${k}.0 > ${k}.tmp
else
	if [ "$IS_SS" -gt 0 ]; then
	awk '{print 4, $0 }' ${k}.0 > ${k}.tmp
	else
		if [ "$IS_TB" -gt 0 ]; then
		awk '{print 5, $0 }' ${k}.0 > ${k}.tmp
		else
		echo " " 
		fi
	fi
fi

####################################################
####################################################
####################################################

awk '

BEGIN {
	mismatch_count = 0;
	count_2d = 0;
	count_ss = 0;
	count_tb = 0;
	count_unk = 0;
	lr_prev_dif = 0;
	tb_prev_dif = 0;
}

# assign parameters
{
	LX_APR_FMT_2D_2D   = 0;
	LX_APR_FMT_3D_SS   = 4;
	LX_APR_FMT_3D_TB   = 5;
	LX_APR_FMT_UNKNOWN = 7;

	APR_IMG_CLASS_TPD     = 0;
	APR_IMG_CLASS_2D_HARD    = 1;
	APR_IMG_CLASS_2D_WEAK_SS = 2;
	APR_IMG_CLASS_2D_WEAK_TB = 3;
	APR_IMG_CLASS_SS_HARD    = 4;
	APR_IMG_CLASS_SS_WEAK    = 5;
	APR_IMG_CLASS_TB_HARD    = 6;
	APR_IMG_CLASS_TB_WEAK    = 7;

	result = LX_APR_FMT_2D_2D;

	p = 1;

	org_fmt = $(p++);
	cur_fmt = $(p++);
	apr_fmt_before_vote = $(p++);
	apr_fmt = $(p++);
	tpd = $(p++);
	ssr = $(p++);
	tbr = $(p++);
	# 8
	thr_subtitle = 192; $(p++);
	thr_variation = 64; $(p++);
	thr_correlation = 248;$(p++);
	thr_corr_3rd = 179;$(p++);
	thr_final_corr = 220;          # larger -> 2D , smaller -> 3D
	thr_mean_tolerance = 10;       # larger -> 3D , smaller -> 2D
	cur_src_fmt = $(p++);
	for(i=0;i<8;i++) mean[i]  = $(p++);
	for(i=0;i<8;i++) s_dev[i] = $(p++);
	scene_type = $(p++);
	bMask = $(p++);
	for(i=0;i<4;i++) corr_ss[i] = $(p++);
	for(i=0;i<4;i++) corr_tb[i] = $(p++);
	for(i=0;i<4;i++) ss_wnd_info[i] = $(p++);
	for(i=0;i<4;i++) tb_wnd_info[i] = $(p++);
	$(p++);
	# 50
	$(p++);
	lrc_lr_diff = $(p++);
	lrc_tb_diff = $(p++);
	for(i=0;i<9;i++) src_tb_diff[i] = $(p++);
	for(i=0;i<9;i++) src_lr_diff[i] = $(p++);
	src_tb_ratio = $(p++);
	src_lr_ratio = $(p++);
	m1_img_class = $(p++);
	hsv_max      = $(p++);
	hsv_count    = $(p++);
	tb_ratio     = $(p++);
	ss_ratio     = $(p++);
	for(i=0;i<4;i++) seg_hsv_count[i] = $(p++);
	for(i=0;i<4;i++) seg_hsv_max[i]   = $(p++);
	motion_l     = $(p++);
	hfd_1_fmt    = $(p++);
	hfd_2_fmt    = $(p++);
	hfd_3_fmt    = $(p++);
	hfd_3_1_fmt    = $(p++);
	img_width    = 1920; $(p++);
	img_height   = 1080; $(p++);
	# 93
}

# display raw data with NR & NF
{
#print NR,NF,": ", apr_fmt_before_vote,thr_subtitle, mean[1],s_dev[0], tb_wnd_info[3]
#print "[",NR,"] ",NF," - ", $0
}

## Image classification by histogram
{
	thr_adj = (tb_ratio * 1024) / 1036800;

	thr1 = 7;
	thr2 = 8;
	if(1)
	{
		thr3 = ((100000 * thr_adj) / 2) / 1024;
		thr4 = ((2000 * thr_adj)  / 2) / 1024;
		thr6 = ((30000 * thr_adj) / 2) / 1024;
		thr7 = ((1500000 * thr_adj) / 2) / 1024;
		thr_const0 = ((2000 * thr_adj) / 2) / 1024;
		thr_const1 = ((5000 * thr_adj) / 2) / 1024;
	}
	else
	{
		thr3 = 100000 /2;
		thr4 = 2000 / 2;
		thr6 = 30000 / 2;
		thr7 = 1500000 / 2;
		thr_const0 = 2000 / 2;
		thr_const1 = 5000 / 2;
	}
	MIN_TB_SS_RATIO_THR = 5;
	MAX_HISTOGRAM_ON_CASE3 = 1000000;
	go_decide_flag = 0;

#	if((tpd > 182) && ((motion_l < 120) || (motion_l == 255)))
#	{
#			hist_result = LX_APR_FMT_2D_2D;
#			hist_img_class = APR_IMG_CLASS_TPD;
#	}
#	else
	{
		# Histogram diff measure
		if((lrc_lr_diff >= thr3 && lrc_tb_diff >= thr3) || (lrc_lr_diff < thr3 && lrc_tb_diff < thr3))
		{
			hist_result = LX_APR_FMT_2D_2D;
		}
		else
		{
			if(lrc_lr_diff > lrc_tb_diff)
			{
				if((lrc_lr_diff - lrc_tb_diff) < thr4)
					hist_result = LX_APR_FMT_2D_2D;
				else
					hist_result = LX_APR_FMT_3D_TB;
			}
			else
			{
				if((lrc_tb_diff - lrc_lr_diff) < thr4)
					hist_result = LX_APR_FMT_2D_2D;
				else
					hist_result = LX_APR_FMT_3D_SS;
			}
		}

		if (hist_result == LX_APR_FMT_2D_2D)
		{
			if(lrc_lr_diff > lrc_tb_diff)
			{
				if(lrc_tb_diff > thr7)
				{
					hist_img_class = APR_IMG_CLASS_2D_HARD;
					go_decide_flag = 1;
				}
				else if(lrc_tb_diff < thr_const0)
				{
					hist_result = LX_APR_FMT_3D_TB;
					hist_img_class = APR_IMG_CLASS_TB_HARD;
					go_decide_flag = 1;
				}
			}
			else
			{
				if(lrc_lr_diff > thr7)
				{
					hist_img_class = APR_IMG_CLASS_2D_HARD;
					go_decide_flag = 1;
				}
				else if(lrc_lr_diff < thr_const0)
				{
					hist_result = LX_APR_FMT_3D_SS;
					hist_img_class = APR_IMG_CLASS_SS_HARD;
					go_decide_flag = 1;
				}
			}
		}
		else if(hist_result == LX_APR_FMT_3D_SS && lrc_lr_diff < thr6)
		{
			if(lr_prev_dif >= thr6)
			{
				hist_result = LX_APR_FMT_2D_2D;
			}
			if(lrc_tb_diff > MAX_HISTOGRAM_ON_CASE3)
			{
				hist_result = LX_APR_FMT_2D_2D;
			}
			if(hist_result == LX_APR_FMT_2D_2D)
				hist_img_class = APR_IMG_CLASS_2D_WEAK_SS;
			else
				hist_img_class = APR_IMG_CLASS_SS_WEAK;
			go_decide_flag = 1;
		}
		else if(hist_result == LX_APR_FMT_3D_TB && lrc_tb_diff < thr6)
		{
			if(tb_prev_dif >= thr6)
			{
				hist_result = LX_APR_FMT_2D_2D;
			}
			if(lrc_lr_diff > MAX_HISTOGRAM_ON_CASE3)
			{
				hist_result = LX_APR_FMT_2D_2D;
			}
			if(hist_result == LX_APR_FMT_2D_2D)
				hist_img_class = APR_IMG_CLASS_2D_WEAK_TB;
			else
				hist_img_class = APR_IMG_CLASS_TB_WEAK;
			go_decide_flag = 1;
		}

		if(go_decide_flag == 0)
		{
			# Delta measure
			if(hist_result == LX_APR_FMT_2D_2D)
			{
				if(ssr > tbr)
				{
					if(ssr > thr2 && (lrc_lr_diff + thr_const1) < lrc_tb_diff) 
					{
						hist_result = LX_APR_FMT_3D_SS;
						hist_img_class = APR_IMG_CLASS_SS_HARD;
					}
					else
					{
						hist_img_class = APR_IMG_CLASS_2D_WEAK_SS;
					}
				}
				else if(ssr < tbr)
				{
					if(tbr > thr2 && (lrc_tb_diff + thr_const1) < lrc_lr_diff)
					{
						hist_result = LX_APR_FMT_3D_TB;
						hist_img_class = APR_IMG_CLASS_TB_HARD;
					}
					else
					{
						hist_img_class = APR_IMG_CLASS_2D_WEAK_TB;
					}
				}
				else
				{
					hist_img_class = APR_IMG_CLASS_2D_HARD;
				}
			}
			else
			{
				if(ssr > tbr && lrc_lr_diff < lrc_tb_diff)
				{
					if(ssr > thr1)
					{
						if(hist_result != LX_APR_FMT_3D_SS)
						{
							hist_result = LX_APR_FMT_2D_2D;
							hist_img_class = APR_IMG_CLASS_2D_WEAK_SS;
						}
						else
						{
							hist_img_class = APR_IMG_CLASS_SS_HARD;
						}
					}
					else if(ssr - tbr <= MIN_TB_SS_RATIO_THR)
					{
						hist_result = LX_APR_FMT_2D_2D;
						hist_img_class = APR_IMG_CLASS_2D_WEAK_SS;
					}
				}
				else if(tbr > ssr && lrc_tb_diff < lrc_lr_diff)
				{
					if(tbr > thr1)
					{
						if(hist_result != LX_APR_FMT_3D_TB)
						{
							hist_result = LX_APR_FMT_2D_2D;
							hist_img_class = APR_IMG_CLASS_2D_WEAK_TB;
						}
						else
						{
							hist_img_class = APR_IMG_CLASS_TB_HARD;
						}
					}
					else if(tbr - ssr <= MIN_TB_SS_RATIO_THR)
					{
						hist_result = LX_APR_FMT_2D_2D;
						hist_img_class = APR_IMG_CLASS_2D_WEAK_TB;
					}
				}
				else
				{
					hist_result = LX_APR_FMT_2D_2D;
					hist_img_class = APR_IMG_CLASS_2D_HARD;
				}
			}
		}
		lr_prev_dif = lrc_lr_diff;
		tb_prev_dif = lrc_tb_diff;
	}
}

## calculate with decision rule
{
	# subtitle mode
	if(scene_type == 1)
	{
		final_score_ss = 0; 
		final_score_tb = 0; 
		final_cnt_ss = 0;
		final_cnt_tb = 0;
		final_th_ss = 0;
		final_th_tb = 0;

		for(i=0;i<4;i++)
		{
			if(!ss_wnd_info[i])
			{
				final_cnt_ss++;
				final_score_ss += corr_ss[i];
			}
			if(!tb_wnd_info[i])
			{
				final_cnt_tb++;
				final_score_tb += corr_tb[i];
			}
		}

		final_th_ss = thr_subtitle * final_cnt_ss;
		final_th_tb = thr_subtitle * final_cnt_tb;

		if(!final_cnt_ss) final_score_ss = -1;
		if(!final_cnt_tb) final_score_tb = -1;

		if((!final_cnt_ss) && (!final_cnt_tb))
		{
			result = LX_APR_FMT_UNKNOWN;
		}
		else
		{
			if((final_score_ss >= final_th_ss) && (final_score_tb >= final_th_tb))
				result = LX_APR_FMT_UNKNOWN;
			else if(final_score_ss >= final_th_ss)
				result = LX_APR_FMT_3D_SS;
			else if(final_score_tb >= final_th_tb)
				result = LX_APR_FMT_3D_TB;
			else
				result = LX_APR_FMT_2D_2D;
		}
		#print "Subtuitle - ", result, final_score_ss, final_th_ss, final_score_tb, final_th_tb 
	}
}

{
	if (hist_img_class == APR_IMG_CLASS_2D_HARD)
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 200;
		thr_final_corr     = 220; 
		thr_mean_tolerance = 20;
		thr_variation      = 64;
		thr_valid_blk      = 1;
		candi_blk_thaw     = 0;
		cross_corr_en      = 1;
		cross_corr_th0     = 30;
		cross_corr_th1     = 50;
		dom_seg_chk_en     = 1;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512; 
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 1;
	}
	else if (hist_img_class == APR_IMG_CLASS_TB_HARD)
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 160;
		thr_final_corr     = 200; 
		thr_mean_tolerance = 20;
		thr_variation      = 0;
		thr_valid_blk      = 2;
		candi_blk_thaw     = 1;
		cross_corr_en      = 0;
		cross_corr_th0     = 0;
		cross_corr_th1     = 0;
		dom_seg_chk_en     = 0;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512;
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 0;
	}
	else if (hist_img_class == APR_IMG_CLASS_SS_HARD)
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 160;
		thr_final_corr     = 200; 
		thr_mean_tolerance = 20;
		thr_variation      = 64;
		thr_valid_blk      = 2;
		candi_blk_thaw     = 1;
		cross_corr_en      = 0;
		cross_corr_th0     = 0;
		cross_corr_th1     = 0;
		dom_seg_chk_en     = 0;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512;
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 0;
	}
	else if ((hist_img_class == APR_IMG_CLASS_TB_WEAK) || (hist_img_class == APR_IMG_CLASS_2D_WEAK_TB))
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 179;
		thr_final_corr     = 220; 
		thr_mean_tolerance = 20;
		thr_variation      = 0;
		thr_valid_blk      = 1;
		candi_blk_thaw     = 0;
		cross_corr_en      = 1;
		cross_corr_th0     = 30;
		cross_corr_th1     = 50;
		dom_seg_chk_en     = 1;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512;
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 1;
	}
	else if ((hist_img_class == APR_IMG_CLASS_SS_WEAK) || (hist_img_class == APR_IMG_CLASS_2D_WEAK_SS))
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 179;
		thr_final_corr     = 220; 
		thr_mean_tolerance = 20;
		thr_variation      = 0;
		thr_valid_blk      = 1;
		candi_blk_thaw     = 0;
		cross_corr_en      = 1;
		cross_corr_th0     = 30;
		cross_corr_th1     = 50;
		dom_seg_chk_en     = 1;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512;
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 1;
	}
	else
	{
		thr_correlation    = 248;
		thr_corr_3rd       = 179;
		thr_final_corr     = 220; 
		thr_mean_tolerance = 20;
		thr_variation      = 0;
		thr_valid_blk      = 1;
		candi_blk_thaw     = 0;
		cross_corr_en      = 0;
		cross_corr_th0     = 0;
		cross_corr_th1     = 0;
		dom_seg_chk_en     = 1;
		dom_seg_chk_th0    = (img_width * img_height * 3) / 512;
		dom_seg_chk_th1    = (img_width * img_height * 3) / 256;
		blk_mean_chk_en    = 1;
	}
		
	cross_corr_en = 0;

	# video mode
	if(scene_type == 0)
	{
		cnt_ss = 0; cnt1_ss = 0; cnt2_ss = 0;
		cnt_tb = 0; cnt1_tb = 0; cnt2_tb = 0;
		cnt_idx = 0;
		sum_ss = 0; sum_tb = 0;
		final_corr = 0;	
		scan_3d[0] = 0;	scan_3d[1] = 1; scan_3d[2] = 2; scan_3d[3] = 3;
		corr_3d[0] = 0; corr_3d[1] = 0; corr_3d[2] = 0; corr_3d[3] = 0;

		swi_val = 0;
		swi_idx = 0;

		mean_d[0] = 0; mean_d[1] = 0; mean_d[2] = 0; mean_d[3] = 0;
		mean_d1[0] = 0; mean_d1[1] = 0; mean_d1[2] = 0; mean_d1[3] = 0;

		tmp_mean_a = 0; tmp_mean_b = 0;
		match_cor_mean = 1;
		match_max_d = 0;
		di = 0; di_a = 0;

		tmp_idx = 0;
		case_flag = 0;
		detect_format = LX_APR_FMT_2D_2D;

		# 1st decision - basic
		{
			# condition check (SS)
			for (i=0; i<4; i++)
			{
				tmp_idx = (i<2) ? i : i+2;

				if (ss_wnd_info[i] == 0)
				{
					cnt1_ss++;
					if ( (s_dev[tmp_idx]   >= thr_variation) &&	(s_dev[tmp_idx+2] >= thr_variation) )
					{
						cnt_ss++;			
						sum_ss += corr_ss[i];
					}
				}

				if (ss_wnd_info[i] == 2) cnt2_ss++;
			}

			# condition check (TB)
			for (i=0; i<4; i++)
			{
				tmp_idx = i;

				if (tb_wnd_info[i] == 0)
				{
					cnt1_tb++;
					cnt_tb++;
					sum_tb += corr_tb[i];
				}

				if (tb_wnd_info[i] == 2) cnt2_tb++;
			}

			sum_ss = (cnt_ss == 0) ? 0 : sum_ss;
			sum_tb = (cnt_tb == 0) ? 0 : sum_tb;

			# format decision based on correlation
			if      (                 (cnt1_tb < 2) && (cnt1_ss < 2)) {
				detect_format = LX_APR_FMT_2D_2D;
				if(candi_blk_thaw) change_segment = 1;
			}
			else if ((bMask == 31) && (cnt_tb  < 2) && (cnt_ss  < 2)) detect_format = LX_APR_FMT_UNKNOWN;
			else if (                 (cnt_tb == 0) && (cnt_ss == 0)) detect_format = LX_APR_FMT_2D_2D;
			else
			{
				if      ((cnt_tb == 0) && (cnt_ss ==0)) case_flag = 0;
				else if ((cnt_tb == 0)                ) case_flag = 1;
				else if ((cnt_ss == 0)                ) case_flag = 2;		
				else 					case_flag = 3;	

				if (case_flag == 0)
				{

					if (cnt2_ss > thr_valid_blk)           detect_format = LX_APR_FMT_2D_2D;
					else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_2D_2D;

					cnt_idx    = cnt_ss;
					final_corr = sum_ss;
				}
				else if (case_flag == 1)
				{
					if (cnt2_ss > thr_valid_blk) {
						detect_format = LX_APR_FMT_2D_2D;
						change_segment = 1;
					}
					else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_3D_SS;

					cnt_idx    = cnt_ss;
					final_corr = sum_ss;
				}
				else if (case_flag == 2)
				{
					if (cnt2_tb > thr_valid_blk) {
						detect_format = LX_APR_FMT_2D_2D;
						change_segment = 1;
					}
					else if (sum_ss > thr_correlation * cnt_ss) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_3D_TB;

					cnt_idx    = cnt_tb;
					final_corr = sum_tb;
				}
				else
				{
					if (sum_tb*cnt_ss > sum_ss*cnt_tb)
					{
						if (cnt2_tb > thr_valid_blk) {
							detect_format = LX_APR_FMT_2D_2D;
							change_segment = 1;
						}
						else if (sum_ss > thr_correlation * cnt_ss) detect_format = LX_APR_FMT_2D_2D;
						else                       detect_format = LX_APR_FMT_3D_TB;

						cnt_idx    = cnt_tb;
						final_corr = sum_tb;
					}
					else
					{
						if (cnt2_ss > thr_valid_blk) {
							detect_format = LX_APR_FMT_2D_2D;
							change_segment = 1;
						}
						else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
						else                       detect_format = LX_APR_FMT_3D_SS;

						cnt_idx    = cnt_ss;
						final_corr = sum_ss;
					}
				}
			}
			
			# added new alg. 20121207
			{
				diff_corr_tb[0] = 0;
				diff_corr_tb[1] = 0;
				diff_corr_ss[0] = 0;
				diff_corr_ss[1] = 0;
				cross_corr_th[0] = 0;
				cross_corr_th[1] = 0;
				large_corr_tb[0] = 0;
				large_corr_tb[1] = 0;
				large_corr_ss[0] = 0;
				large_corr_ss[1] = 0;

				diff_corr_tb[0] = corr_tb[0] - corr_tb[2];
				diff_corr_tb[1] = corr_tb[1] - corr_tb[3];

				diff_corr_ss[0] = corr_ss[0] - corr_ss[2];
				diff_corr_ss[1] = corr_ss[1] - corr_ss[3];

				diff_corr_tb[0] = (diff_corr_tb[0] < 0) ? -diff_corr_tb[0] : diff_corr_tb[0];
				diff_corr_tb[1] = (diff_corr_tb[1] < 0) ? -diff_corr_tb[1] : diff_corr_tb[1];
				diff_corr_ss[0] = (diff_corr_ss[0] < 0) ? -diff_corr_ss[0] : diff_corr_ss[0];
				diff_corr_ss[1] = (diff_corr_ss[1] < 0) ? -diff_corr_ss[1] : diff_corr_ss[1];

				large_corr_tb[0] = (corr_tb[0] >  corr_tb[2]) ? corr_tb[0] : corr_tb[1];
				large_corr_ss[0] = (corr_ss[0] >  corr_ss[2]) ? corr_ss[0] : corr_ss[1];

				if (cross_corr_en == 1)
				{
					if (detect_format == LX_APR_FMT_3D_SS) 
					{
						cross_corr_th[0] = large_corr_tb[0] < 100 ? cross_corr_th0 : cross_corr_th1;
						cross_corr_th[1] = large_corr_tb[1] < 100 ? cross_corr_th0 : cross_corr_th1;

						if (diff_corr_tb[0] > cross_corr_th[0] || diff_corr_tb[1] > cross_corr_th[1])
						{
							detect_format = LX_APR_FMT_2D_2D;
						}
					}
					else if (detect_format == LX_APR_FMT_3D_TB)
					{
						cross_corr_th[0] = large_corr_ss[0] < 100 ? cross_corr_th0 : cross_corr_th1;
						cross_corr_th[1] = large_corr_ss[1] < 100 ? cross_corr_th0 : cross_corr_th1;

						if (diff_corr_ss[0] > cross_corr_th[0] || diff_corr_ss[1] > cross_corr_th[1])
						{
							detect_format = LX_APR_FMT_2D_2D;
						}
					}

				}
			}
		}
		
		sim_1_fmt = detect_format;

		# 2nd decision
		{
			if ( (detect_format == LX_APR_FMT_3D_TB) || (detect_format == LX_APR_FMT_3D_SS) )
			{
				if (detect_format == LX_APR_FMT_3D_TB)
				{
					for (i=0; i<4; i++) corr_3d[i] = (tb_wnd_info[i]==0) ? corr_tb[i] : -1;				 
				}
				else
				{
					for (i=0; i<4; i++) corr_3d[i] = (ss_wnd_info[i]==0) ? corr_ss[i] : -1; 
				}

				for (iter=0; iter<3; iter++)
				{
					for (i=0; i<3; i++)
					{
						if (corr_3d[i] < corr_3d[i+1])
						{
							swi_val      = corr_3d[i];
							corr_3d[i]   = corr_3d[i+1];
							corr_3d[i+1] = swi_val;

							swi_idx      = scan_3d[i];
							scan_3d[i]   = scan_3d[i+1];
							scan_3d[i+1] = swi_idx;
						}
					}
				}

				if (corr_3d[2] < 0) 
				{
					corr_3d[2] = (corr_3d[1] < 0) ? corr_3d[0] : corr_3d[1];
				}

				if (corr_3d[2] < thr_corr_3rd)
				{
					detect_format = LX_APR_FMT_2D_2D;
				}
			}
		}
		
		sim_2_fmt = detect_format;
		
		# 3rd
		{
			if (detect_format == LX_APR_FMT_3D_TB)
			{
				for (i=0; i<4; i++)
				{
					tmp_idx = i;
					tmp_mean_a = mean[tmp_idx  ];
					tmp_mean_b = mean[tmp_idx+4];

					mean_d[i] = (tmp_mean_a > tmp_mean_b) ? (tmp_mean_a - tmp_mean_b) : (tmp_mean_b - tmp_mean_a);
				}

				for (i=0; i<4; i++) mean_d1[i] = mean_d[scan_3d[i]];
			}
			else if (detect_format == LX_APR_FMT_3D_SS)
			{
				for (i=0; i<4; i++)
				{
					tmp_idx = (i<2) ? i : i+2;
					tmp_mean_a = mean[tmp_idx  ];
					tmp_mean_b = mean[tmp_idx+2];

					mean_d[i] = (tmp_mean_a > tmp_mean_b) ? (tmp_mean_a - tmp_mean_b) : (tmp_mean_b - tmp_mean_a);
				}

				for (i=0; i<4; i++) mean_d1[i] = mean_d[scan_3d[i]];
			}
			else
			{
				for (i=0; i<4; i++) mean_d1[i] = 0;
			}

			match_cor_mean = 1;
			match_max_d    = 0;

			for (i=3; i>0; i--)
			{
				di   = mean_d1[i] - mean_d1[i-1];
				di_a = (di<0) ? -di : di;

				if (di_a > match_max_d) match_max_d = di_a;
				if (di < -20*16) match_cor_mean = 0;
			}

			if ( (detect_format != LX_APR_FMT_2D_2D) && (final_corr < thr_correlation*cnt_idx) && (bMask == 31) )
			{
				if (!match_cor_mean)
					detect_format = LX_APR_FMT_2D_2D;
				else if ( (final_corr > thr_final_corr*cnt_idx) && (match_max_d< thr_mean_tolerance*16) )
					detect_format = detect_format;
				else
					detect_format += 16;
			}
		}
		
		sim_3_1_fmt = detect_format;
		
		{
			if (dom_seg_chk_en==1)
			{
				seg_idx[0] = 0; seg_idx[1] = 0; seg_idx[2] = 0; seg_idx[3] = 0;
				seg_max[0] = 0; seg_max[1] = 0; seg_max[2] = 0; seg_max[3] = 0;
				seg_max_di[0] = 0; seg_max_di[1] = 0;
				seg_idx_flag[0] = 0; seg_idx_flag[1] = 0;

				for (i=0; i<4; i++) 
				{
					seg_idx[i] = seg_hsv_max[i];
					seg_max[i] = seg_hsv_count[i];
				}

				if (detect_format == LX_APR_FMT_3D_SS) 
				{
					seg_idx_flag[0] = (seg_idx[0] < 12) ? 0 : 1;
					seg_idx_flag[1] = (seg_idx[2] < 12) ? 0 : 1;

					if (seg_idx[0]==seg_idx[1]) 
						seg_max_di[0] = (seg_max[0]>seg_max[1]) ? (seg_max[0] - seg_max[1]) : (seg_max[1] - seg_max[0]);
					else if (( seg_idx[0]>=12) || (seg_idx[1] >= 12))
						seg_max_di[0] = -2;
					else
						seg_max_di[0] = -1;

					if (seg_idx[2]==seg_idx[3]) 
						seg_max_di[1] = (seg_max[2]>seg_max[3]) ? (seg_max[2] - seg_max[3]) : (seg_max[3] - seg_max[2]);
					else if (( seg_idx[2]>=12) || (seg_idx[3] >= 12))
						seg_max_di[1] = -2;
					else
						seg_max_di[1] = -1;
				}
				else if (detect_format == LX_APR_FMT_3D_TB)
				{
					seg_idx_flag[0] = (seg_idx[0] < 12) ? 0 : 1;
					seg_idx_flag[1] = (seg_idx[1] < 12) ? 0 : 1;

					if (seg_idx[0]==seg_idx[2]) 
						seg_max_di[0] = (seg_max[0]>seg_max[2]) ? (seg_max[0] - seg_max[2]) : (seg_max[2] - seg_max[0]);
					else if (( seg_idx[0]>=12) || (seg_idx[2] >= 12))
						seg_max_di[0] = -2;
					else
						seg_max_di[0] = -1;

					if (seg_idx[1]==seg_idx[3]) 
						seg_max_di[1] = (seg_max[1]>seg_max[3]) ? (seg_max[1] - seg_max[3]) : (seg_max[3] - seg_max[1]);
					else if (( seg_idx[1]>=12) || (seg_idx[3] >= 12))
						seg_max_di[1] = -2;
					else
						seg_max_di[1] = -1;
				}
				else
				{
					seg_max_di[0] = -100;
					seg_max_di[1] = -100;
				}

				dom_seg_chk_th[0] = (seg_idx_flag[0] == 0) ? dom_seg_chk_th1 : dom_seg_chk_th0;
				dom_seg_chk_th[1] = (seg_idx_flag[1] == 0) ? dom_seg_chk_th1 : dom_seg_chk_th0;

				if ( (seg_idx[0] != 0) && (seg_max_di[0] > dom_seg_chk_th[0]))
				{	
					detect_format = LX_APR_FMT_2D_2D;
				}
				else if ( (seg_idx[1] != 0) && (seg_max_di[1] > dom_seg_chk_th[1]))
				{
					detect_format = LX_APR_FMT_2D_2D;
				}
			}

			if (blk_mean_chk_en == 1)
			{
				blk_mean[0] = 0; blk_mean[1] = 0; blk_mean[2] = 0; blk_mean[3] = 0;
				blk_mean[4] = 0; blk_mean[5] = 0; blk_mean[6] = 0; blk_mean[7] = 0;
				L_census[0] = 0; L_census[1] = 0; L_census[2] = 0; L_census[3] = 0;
				R_census[0] = 0; R_census[1] = 0; R_census[2] = 0; R_census[3] = 0;

				ref_mean = 0;

				blk_mean0[0] = 0; blk_mean0[1] = 0; blk_mean0[2] = 0; blk_mean0[3] = 0;
				blk_mean1[0] = 0; blk_mean1[1] = 0; blk_mean1[2] = 0; blk_mean1[3] = 0;
				blk_idx0[0] = 0; blk_idx0[1] = 0; blk_idx0[2] = 0; blk_idx0[3] = 0;
				blk_idx1[0] = 0; blk_idx1[1] = 0; blk_idx1[2] = 0; blk_idx1[3] = 0;

				Hamming_Distance = 0;

				for (m=0; m<8; m++)
				{
					blk_mean[m] = mean[m];
				}

				if (bMask == 31)  
				{
					if ((detect_format == LX_APR_FMT_3D_SS) && (cnt2_ss == 0))
					{				
						# copy 
						for (m=0; m<4; m++)
						{
							blk_mean0[m] = (m>1) ? blk_mean[m+2] : blk_mean[m];
							blk_mean1[m] = (m>1) ? blk_mean[m+4] : blk_mean[m+2];
							blk_idx0[m]  = m;
							blk_idx1[m]  = m;
						}

						# sorting 
						for (iter=0; iter<3; iter++)
						{
							for (i=0; i<3; i++)
							{
								if (blk_mean0[i] < blk_mean0[i+1])
								{
									swi_val      	= blk_mean0[i];
									blk_mean0[i]   	= blk_mean0[i+1];
									blk_mean0[i+1] 	= swi_val;

									swi_idx       = blk_idx0[i];
									blk_idx0[i]   = blk_idx0[i+1];
									blk_idx0[i+1] = swi_idx;
								}

								if (blk_mean1[i] < blk_mean1[i+1])
								{
									swi_val      	= blk_mean1[i];
									blk_mean1[i]   	= blk_mean1[i+1];
									blk_mean1[i+1] 	= swi_val;

									swi_idx       = blk_idx1[i];
									blk_idx1[i]   = blk_idx1[i+1];
									blk_idx1[i+1] = swi_idx;
								}
							}
						}

						ref_mean = blk_mean0[3];
						ref_mean += 320; 

						L_census[0] = (blk_mean[0] > ref_mean ) ? 1 : 0;
						L_census[1] = (blk_mean[1] > ref_mean ) ? 1 : 0;
						L_census[2] = (blk_mean[4] > ref_mean ) ? 1 : 0;
						L_census[3] = (blk_mean[5] > ref_mean ) ? 1 : 0;

						ref_mean = blk_mean1[3];
						ref_mean += 320;

						R_census[0] = (blk_mean[2] > ref_mean ) ? 1 : 0;
						R_census[1] = (blk_mean[3] > ref_mean ) ? 1 : 0;
						R_census[2] = (blk_mean[6] > ref_mean ) ? 1 : 0;
						R_census[3] = (blk_mean[7] > ref_mean ) ? 1 : 0;

						# Hamming_Distance
						for (m=0; m<4; m++)
						{
							if (L_census[m] == R_census[m]) Hamming_Distance += 1;
						}

						if (Hamming_Distance < 3) detect_format = LX_APR_FMT_2D_2D;
					}
					else if ((detect_format == LX_APR_FMT_3D_TB) && (cnt2_tb == 0))
					{ 
						# copy 
						for (m=0; m<4; m++)
						{
							blk_mean0[m] = blk_mean[m];
							blk_mean1[m] = blk_mean[m+4];
							blk_idx0[m]  = m;
							blk_idx1[m]  = m;
						}

						# sorting 
						for (iter=0; iter<3; iter++)
						{
							for (i=0; i<3; i++)
							{
								if (blk_mean0[i] < blk_mean0[i+1])
								{
									swi_val      	= blk_mean0[i];
									blk_mean0[i]   	= blk_mean0[i+1];
									blk_mean0[i+1] 	= swi_val;

									swi_idx       = blk_idx0[i];
									blk_idx0[i]   = blk_idx0[i+1];
									blk_idx0[i+1] = swi_idx;
								}

								if (blk_mean1[i] < blk_mean1[i+1])
								{
									swi_val      	= blk_mean1[i];
									blk_mean1[i]   	= blk_mean1[i+1];
									blk_mean1[i+1] 	= swi_val;

									swi_idx       = blk_idx1[i];
									blk_idx1[i]   = blk_idx1[i+1];
									blk_idx1[i+1] = swi_idx;
								}
							}
						}

						ref_mean = blk_mean0[3];
						ref_mean += 320; 

						L_census[0] = (blk_mean[0] > ref_mean ) ? 1 : 0;
						L_census[1] = (blk_mean[1] > ref_mean ) ? 1 : 0;
						L_census[2] = (blk_mean[2] > ref_mean ) ? 1 : 0;
						L_census[3] = (blk_mean[3] > ref_mean ) ? 1 : 0;

						ref_mean = blk_mean1[3];
						ref_mean += 320; 

						R_census[0] = (blk_mean[4] > ref_mean ) ? 1 : 0;
						R_census[1] = (blk_mean[5] > ref_mean ) ? 1 : 0;
						R_census[2] = (blk_mean[6] > ref_mean ) ? 1 : 0;
						R_census[3] = (blk_mean[7] > ref_mean ) ? 1 : 0;

						# Hamming_Distance
						for (m=0; m<4; m++)
						{
							if (L_census[m] == R_census[m]) Hamming_Distance += 1;
						}

						if (Hamming_Distance < 3) detect_format = LX_APR_FMT_2D_2D;
					}
				}
			}
		}
		
		sim_3_fmt = detect_format;

		if(tpd >= 250)	
			result = LX_APR_FMT_2D_2D;
		else
			result = (detect_format >= 16) ? LX_APR_FMT_UNKNOWN : detect_format;

	#	print "Videomode - ", result, detect_format, final_corr, thr_correlation
#	if((result != LX_APR_FMT_UNKNOWN) && (org_fmt != result))
		if((org_fmt != result))
		{
#			printf("%d) hist %d , fd3 %d - %d %d %d %d %d\n", NR, hist_result, result, tpd, ssr, tbr, lrc_lr_diff, lrc_tb_diff);
		}
	}
}

{

	if(result != apr_fmt_before_vote)
	{
		mismatch_count++;
#		printf("  %d) mismatch : lrc_hst[%d] m1_img_class[%d] fd3[%d] =>  sim_hst[%d] sim_img_class[%d] sim_result[%d] scene[%d]\n", NR, cur_fmt, m1_img_class, apr_fmt_before_vote, hist_result, hist_img_class, result, scene_type);
#		printf("      stage fmt : 1[%d/%d] 2[%d/%d] 3[%d/%d] 3-1[%d/%d] match_cor_mean[%d] %d %di  \n", hfd_1_fmt, sim_1_fmt, hfd_2_fmt, sim_2_fmt, hfd_3_fmt, sim_3_fmt, hfd_3_1_fmt, sim_3_1_fmt, match_cor_mean,  corr_3d[2], thr_corr_3rd );
			
#		printf("      lrc_lr_diff[%d] lrc_tb_diff[%d] thr7[%d] ssr[%d] tbr[%d]\n", lrc_lr_diff, lrc_tb_diff, thr7, ssr, tbr);
	}
	if(result == LX_APR_FMT_2D_2D)
		count_2d++;
	if(result == LX_APR_FMT_3D_SS)
		count_ss++;
	if(result == LX_APR_FMT_3D_TB) 
		count_tb++;
	if(result == LX_APR_FMT_UNKNOWN)
		count_unk++;
}



END {
	if(org_fmt == LX_APR_FMT_2D_2D)
		print "  2D 0 : Success Rate  ", (count_2d + count_unk)*100/NR, "%", " Total", NR, "- mismatch", mismatch_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk
	if(org_fmt == LX_APR_FMT_3D_SS)
		print "  SS 4 : Success Rate  ", (count_ss + count_unk)*100/NR, "%", " Total", NR, "- mismatch", mismatch_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk
	if(org_fmt == LX_APR_FMT_3D_TB)
		print "  TB 5 : Success Rate  ", (count_tb + count_unk)*100/NR, "%", " Total", NR, "- mismatch", mismatch_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk

}

' ${k}.tmp >> ${RESULT_FILE}


rm ${k}.0
rm ${k}.tmp

done

###########################
### overall result #########
###########################

sed '

#s/\[.*\]*//g
#/^$/d
#/MESSAGE.\_.A$/d
/done/d
/capture/d

' ${RESULT_FILE} > ${RESULT_FILE}.1


awk '
BEGIN {
	total_2d = 0;
	total_ss = 0;
	total_tb = 0;
	total_un = 0;
	match_2d = 0;
	match_ss = 0;
	match_tb = 0;
}
# body
{
	src_fmt   = $2;
	sample_total = $9;
	result_2d = $14;
	result_ss = $15;
	result_tb = $16;
	result_un = $17;

	if(src_fmt == 0)
	{
		total_2d += sample_total;
		match_2d += (result_2d + result_un);
	}
	else if(src_fmt == 4)
	{
		total_ss += sample_total;
		match_ss += (result_ss + result_un);
	}
	else if(src_fmt == 5)
	{
		total_tb += sample_total;
		match_tb += (result_tb + result_un);
	}
}
END {
	printf "\n\nOverall result (success)\n\n\n"
	printf "  %f %% \n\n\n", (match_2d+match_ss+match_tb)*100/(total_2d+total_ss+total_tb)
	printf "  2D : %f %%   (%d / %d)\n", match_2d*100/total_2d, match_2d, total_2d
	printf "  3D : %f %%   (%d / %d)\n\n", (match_ss+match_tb)*100/(total_ss+total_tb), match_ss, total_ss
	printf "       - SS : %f %%    (%d / %d)\n", match_ss*100/total_ss, match_ss, total_ss
	printf "         TB : %f %%    (%d / %d)\n\n", match_tb*100/total_tb, match_tb, total_tb
	print ""

#printf "  %2f %\n", (match_2d+match_ss+match_tb)*100/(total_2d+total_ss+total_tb)
}
' ${RESULT_FILE}.1 >> ${RESULT_FILE}

rm ${RESULT_FILE}.1

echo "done..."
cat ${RESULT_FILE}


exit 0
