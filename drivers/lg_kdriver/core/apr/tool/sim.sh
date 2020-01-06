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
	result_match_count = 0;
	count_2d = 0;
	count_ss = 0;
	count_tb = 0;
	count_unk = 0;
}

# assign parameters
{
	LX_APR_FMT_2D_2D   = 0;
	LX_APR_FMT_3D_SS   = 4;
	LX_APR_FMT_3D_TB   = 5;
	LX_APR_FMT_UNKNOWN = 7;

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
	cur_src_fmt = $(p++);
	mean[0] = $(p++);
	mean[1] = $(p++);
	mean[2] = $(p++);
	mean[3] = $(p++);
	mean[4] = $(p++);
	mean[5] = $(p++);
	mean[6] = $(p++);
	mean[7] = $(p++);
	s_dev[0] = $(p++);
	s_dev[1] = $(p++);
	s_dev[2] = $(p++);
	s_dev[3] = $(p++);
	s_dev[4] = $(p++);
	s_dev[5] = $(p++);
	s_dev[6] = $(p++);
	s_dev[7] = $(p++);
	scene_type = $(p++);
	bMask = $(p++);
	corr_ss[0] = $(p++);
	corr_ss[1] = $(p++);
	corr_ss[2] = $(p++);
	corr_ss[3] = $(p++);
	corr_tb[0] = $(p++);
	corr_tb[1] = $(p++);
	corr_tb[2] = $(p++);
	corr_tb[3] = $(p++);
	ss_wnd_info[0] = $(p++);
	ss_wnd_info[1] = $(p++);
	ss_wnd_info[2] = $(p++);
	ss_wnd_info[3] = $(p++);
	tb_wnd_info[0] = $(p++);
	tb_wnd_info[1] = $(p++);
	tb_wnd_info[2] = $(p++);
	tb_wnd_info[3] = $(p++);
}

# display raw data with NR & NF
{
#print NR,NF,": ", apr_fmt_before_vote,thr_subtitle, mean[1],s_dev[0], tb_wnd_info[3]
#print "[",NR,"] ",NF," - ", $0
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
					if ( (s_dev[tmp_idx]   >= thr_variation) && (s_dev[tmp_idx+4] >= thr_variation) )
					{
						cnt_tb++;
						sum_tb += corr_tb[i];
					}
				}

				if (tb_wnd_info[i] == 2) cnt2_tb++;
			}

			sum_ss = (cnt_ss == 0) ? 0 : sum_ss;
			sum_tb = (cnt_tb == 0) ? 0 : sum_tb;

			# format decision based on correlation
			if      (                 (cnt1_tb < 2) && (cnt1_ss < 2)) detect_format = LX_APR_FMT_2D_2D;
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

					if (cnt2_ss > 1)           detect_format = LX_APR_FMT_2D_2D;
					else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_3D_SS;

					cnt_idx    = cnt_ss;
					final_corr = sum_ss;
				}
				else if (case_flag == 1)
				{
					if (cnt2_ss > 1)           detect_format = LX_APR_FMT_2D_2D;
					else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_3D_SS;

					cnt_idx    = cnt_ss;
					final_corr = sum_ss;
				}
				else if (case_flag == 2)
				{
					if (cnt2_tb > 1)           detect_format = LX_APR_FMT_2D_2D;
					else if (sum_ss > thr_correlation * cnt_ss) detect_format = LX_APR_FMT_2D_2D;
					else                       detect_format = LX_APR_FMT_3D_TB;

					cnt_idx    = cnt_tb;
					final_corr = sum_tb;
				}
				else
				{
					if (sum_tb*cnt_ss > sum_ss*cnt_tb)
					{
						if (cnt2_tb > 1)           detect_format = LX_APR_FMT_2D_2D;
						else if (sum_ss > thr_correlation * cnt_ss) detect_format = LX_APR_FMT_2D_2D;
						else                       detect_format = LX_APR_FMT_3D_TB;

						cnt_idx    = cnt_tb;
						final_corr = sum_tb;
					}
					else
					{
						if (cnt2_ss > 1)           detect_format = LX_APR_FMT_2D_2D;
						else if (sum_tb > thr_correlation * cnt_tb) detect_format = LX_APR_FMT_2D_2D;
						else                       detect_format = LX_APR_FMT_3D_SS;

						cnt_idx    = cnt_ss;
						final_corr = sum_ss;
					}
				}
			}
		}

		# 2nd
		{
			if ( (detect_format == LX_APR_FMT_3D_TB) || (detect_format == LX_APR_FMT_3D_SS) )
			{
				if (detect_format == LX_APR_FMT_3D_TB)
				{
					for (i=0; i<4; i++) corr_3d[i] = corr_tb[i];				 
				}
				else
				{
					for (i=0; i<4; i++) corr_3d[i] = corr_ss[i];				 
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
					corr_3d[2] = (corr_3d[1] < 0) ? corr_3d[i] : corr_3d[i];
				}

				if (corr_3d[2] < thr_corr_3rd)
				{
					detect_format = LX_APR_FMT_2D_2D;
				}
			}
		}

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
				else if ( (final_corr > 220*cnt_idx) && (match_max_d< 10*16) )
					detect_format = detect_format;
				else
					detect_format += 16;
			}
		}
		result = (detect_format >= 16) ? LX_APR_FMT_UNKNOWN : detect_format;
	#	print "Videomode - ", result, detect_format, final_corr, thr_correlation
	}
}

{
#if(tpd > 250)	result = LX_APR_FMT_2D_2D;

	if(result != apr_fmt_before_vote)
	{
		result_match_count++;
#print NR, ": ", result, apr_fmt_before_vote, scene_type
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
		print "  2D 0 : Success Rate  ", (count_2d + count_unk)*100/NR, "%", " Total", NR, "- mismatch", result_match_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk
	if(org_fmt == LX_APR_FMT_3D_SS)
		print "  SS 4 : Success Rate  ", (count_ss + count_unk)*100/NR, "%", " Total", NR, "- mismatch", result_match_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk
	if(org_fmt == LX_APR_FMT_3D_TB)
		print "  TB 5 : Success Rate  ", (count_tb + count_unk)*100/NR, "%", " Total", NR, "- mismatch", result_match_count , "   2d/ss/tb/un", count_2d,count_ss,count_tb,count_unk

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
