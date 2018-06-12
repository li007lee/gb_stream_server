//////////////////////////////////////////////////////////////////////////////////
// 版权所有，2009-2013，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： ps.c
// 作者：乔勇
// 版本：   1.0
// 日期：   2013-08-15
// 描述：转PS流
// 历史记录：
/////////////////////////////////////////////////////////////////////////////////

#include "my_include.h"
#include "ps.h"

#define MARKER_BIT   		(1)

#define VIDEO_STREAM 		(0xe0) 
#define AUDIO_STREAM 		(0xc0)

#define PTS_DTS_FLAG 		(3)           //2:只有PTS, 3:有PTS和DTS

#define	PS_PES_PPL_LENGTH	(13)
#define	MAX_PES_PACKET_LENGTH 	(0xffff - PS_PES_PPL_LENGTH)

/////////////////////////////////////////////////////////////////////////////////
//PACK Header设置
/////////////////////////////////////////////////////////////////////////////////
#define PSPH_SET_PSC(pPSPH,val) 	((pPSPH)->psc  	= (val))
#define PSPH_SET_SCRB1(pPSPH,val)	((pPSPH)->scrb1 = (val))
#define PSPH_SET_SCRB2(pPSPH,val)	((pPSPH)->scrb2	= (val))
#define PSPH_SET_SCRE(pPSPH,val) 	((pPSPH)->scre  = (val))
#define PSPH_SET_PMR(pPSPH,val)   	((pPSPH)->pmr   = (val))
#define PSPH_SET_MB(pPSPH,val)   	((pPSPH)->mb	= (val))
#define PSPH_SET_RES(pPSPH,val)   	((pPSPH)->res   = (val))
#define PSPH_SET_PSL(pPSPH,val)   	((pPSPH)->psl	= (val))
#define PSPH_SET_SB(pPSPH,val)   	((pPSPH)->sb	= (val))

/////////////////////////////////////////////////////////////////////////////////
//Map Header设置
/////////////////////////////////////////////////////////////////////////////////
#define PMSH_SET_PSCP(pPMSH,val)	((pPMSH)->pscp  = (val))
#define PMSH_SET_PSCP1(pPMSH,val)	((pPMSH)->pscp1 = (val))
#define PMSH_SET_MSI(pPMSH,val)		((pPMSH)->msi   = (val))
#define PMSH_SET_PSML(pPMSH,val)	((pPMSH)->psml  = (val))
#define PMSH_SET_MB(pPMSH,val)		((pPMSH)->mb   	= (val))
#define PMSH_SET_CNI(pPMSH,val)		((pPMSH)->cni  	= (val))
#define PMSH_SET_RES1(pPMSH,val)	((pPMSH)->res1  = (val))
#define PMSH_SET_PSMV(pPMSH,val)	((pPMSH)->psmv  = (val))
#define PMSH_SET_RES(pPMSH,val)		((pPMSH)->res   = (val))
#define PMSH_SET_PSIL(pPMSH,val)	((pPMSH)->psil  = (val))
#define PMSH_SET_ESML(pPMSH,val)	((pPMSH)->esml  = (val))
#define PMSH_SET_VST(pPMSH,val)		((pPMSH)->vst   = (val))
#define PMSH_SET_VESI(pPMSH,val) 	((pPMSH)->vesi  = (val))
#define PMSH_SET_VESIL(pPMSH,val)	((pPMSH)->vesil = (val))
#define PMSH_SET_AST(pPMSH,val)  	((pPMSH)->ast   = (val))
#define PMSH_SET_AESI(pPMSH,val) 	((pPMSH)->aesi  = (val))
#define PMSH_SET_AESIL(pPMSH,val)	((pPMSH)->aesil = (val))
#define PMSH_SET_UC1(pPMSH,val)  	((pPMSH)->uc1	= (val))
#define PMSH_SET_UC2(pPMSH,val)  	((pPMSH)->uc2	= (val))
#define PMSH_SET_CRC(pPMSH,val)  	((pPMSH)->s32crc	= (val))

/////////////////////////////////////////////////////////////////////////////////
//System Header设置
/////////////////////////////////////////////////////////////////////////////////
#define PSSH_SET_SHSC(pPSSH,val) 	((pPSSH)->shsc  = (val))
#define PSSH_SET_HL(pPSSH,val)   	((pPSSH)->hl 	= (val))
#define PSSH_SET_M1(pPSSH,val)   	((pPSSH)->m1   	= (val))
#define PSSH_SET_RBD(pPSSH,val)  	((pPSSH)->rbd  	= (val))
#define PSSH_SET_M2(pPSSH,val)   	((pPSSH)->m2   	= (val))
#define PSSH_SET_AB(pPSSH,val)   	((pPSSH)->ab  	= (val))
#define PSSH_SET_FF(pPSSH,val)   	((pPSSH)->ff   	= (val))
#define PSSH_SET_CF(pPSSH,val)   	((pPSSH)->cf  	= (val))
#define PSSH_SET_SALF(pPSSH,val) 	((pPSSH)->salf	= (val))
#define PSSH_SET_SVLF(pPSSH,val) 	((pPSSH)->svlf  = (val))
#define PSSH_SET_VB(pPSSH,val)   	((pPSSH)->vb   	= (val))
#define PSSH_SET_PRRF(pPSSH,val) 	((pPSSH)->prrf  = (val))
#define PSSH_SET_RBS(pPSSH,val)  	((pPSSH)->rbs   = (val))
#define PSSH_SET_VSI(pPSSH,val)  	((pPSSH)->vsi  	= (val))
#define PSSH_SET_VPBSB(pPSSH,val)	((pPSSH)->vpbsb	= (val))
#define PSSH_SET_ASI(pPSSH,val)  	((pPSSH)->asi  	= (val))
#define PSSH_SET_APBSB(pPSSH,val)	((pPSSH)->apbsb	= (val))
#define PSSH_SET_UC1(pPSSH,val)  	((pPSSH)->uc1  	= (val))
#define PSSH_SET_UC2(pPSSH,val)  	((pPSSH)->uc2  	= (val))
#define PSSH_SET_UC3(pPSSH,val)  	((pPSSH)->uc3  	= (val))

//PES
#define PESH_SET_PSCP(pPESH,val) ((pPESH)->pscp = (val))
#define PESH_SET_M1(pPESH,val)   ((pPESH)->m1   = (val))
#define PESH_SET_SI(pPESH,val)   ((pPESH)->si   = (val))
#define PESH_SET_PPL(pPESH,val)  ((pPESH)->ppl  = (val))
#define PESH_SET_FF(pPESH,val)   ((pPESH)->ff   = (val))
#define PESH_SET_PSC(pPESH,val)  ((pPESH)->psc  = (val))
#define PESH_SET_PP(pPESH,val)   ((pPESH)->pp   = (val))
#define PESH_SET_DAI(pPESH,val)  ((pPESH)->dai  = (val))
#define PESH_SET_CT(pPESH,val)   ((pPESH)->ct   = (val))
#define PESH_SET_OOC(pPESH,val)  ((pPESH)->ooc  = (val))
#define PESH_SET_PDF(pPESH,val)  ((pPESH)->pdf  = (val))
#define PESH_SET_EF(pPESH,val)   ((pPESH)->ef   = (val))
#define PESH_SET_ERF(pPESH,val)  ((pPESH)->erf  = (val))
#define PESH_SET_DTMF(pPESH,val) ((pPESH)->dtmf = (val))
#define PESH_SET_ACIF(pPESH,val) ((pPESH)->acif = (val))
#define PESH_SET_PCF(pPESH,val)  ((pPESH)->pcf  = (val))
#define PESH_SET_PEF(pPESH,val)  ((pPESH)->pef  = (val))
#define PESH_SET_PHDL(pPESH,val) ((pPESH)->phdl = (val))
#define PESH_SET_PTSP(pPESH,val) ((pPESH)->ptsp = (val))
#define PESH_SET_PTS(pPESH,val)  ((pPESH)->pts  = (val))
#define PESH_SET_DTSP(pPESH,val) ((pPESH)->dtsp = (val))
#define PESH_SET_DTS(pPESH,val)  ((pPESH)->dts  = (val))

/////////////////////////////////////////////////////////////////////////////////
// PS流的Pack Header结构体
/////////////////////////////////////////////////////////////////////////////////
#if 0
typedef struct _tagPS_PACKHEADER
{
	unsigned long   psc            ;		//pack_start_code
	unsigned short  scrb1          ;		//system_clock_reference_base1
	unsigned short  scrb2          ;		//system_clock_reference_base2
	unsigned short  scre           ;		//system_clock_reference_extension
	unsigned short  pmr            ;		//program_mux_rate
	unsigned char   mb             ;		//marker_bit
	#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned char   psl           :3;		//pack_stuffing_length
	unsigned char   res           :5;		//reserved
	#elif (BYTE_ORDER == BIG_ENDIAN)
	unsigned char   res           :5;		//reserved
	unsigned char   psl           :3;		//pack_stuffing_length
	#endif

} __attribute__((packed))PS_PACKHEADER_OBJ, *PS_PACKHEADER_HANDLE;


/////////////////////////////////////////////////////////////////////////////////
// PS流的System Header结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_SYSTEMHEADER
{
	unsigned long   shsc            ;           //system_header_start_code
	unsigned short  hl              ;           //head_length
	unsigned char   m1              ;           //fill marker_bit
	unsigned short  rbd             ;			//rate_bound
	unsigned char   uc1             ;
	unsigned char   uc2             ;
	unsigned char   uc3             ;
	unsigned char   vsi             ;           //video_stream_id
	unsigned short  vpbsb           ;           //video_P-STD_buffer_size_bound
	unsigned char   asi             ;           //aideo_stream_id
	unsigned short  apbsb           ;           //aideo_P-STD_buffer_size_bound
} __attribute__((packed))PS_SYSTEMHEADER_OBJ, *PS_SYSTEMHEADER_HANDLE;

/////////////////////////////////////////////////////////////////////////////////
// PS流的Map Header结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_MAPHEADER
{
	unsigned char   pscp1           ;		//packet_start_code_prefix
	unsigned short  pscp            ;		//packet_start_code_prefix
	unsigned char   msi             ;		//map_stream_id
	unsigned short  psml            ;		//program_stream_map_length
	unsigned char   uc1             ;
	unsigned char   uc2             ;
	unsigned short  psil            ;       //program_stream_info_length
	unsigned short  esml            ;		//elementary_stream_map_length
	unsigned char   vst             ;		//video_stream_type
	unsigned char   vesi            ;		//video_elementary_stream_id
	unsigned short  vesil           ;		//video_elementary_stream_info_length
	unsigned short  fill1           ;
	unsigned short  fill2           ;
	unsigned short  fill3           ;
	unsigned char   ast             ;		//aideo_stream_type
	unsigned char   aesi            ;		//aideo_elementary_stream_id
	unsigned short  aesil           ;		//aideo_elementary_stream_info_length
	unsigned long   s32crc          ;		//32位校验和
} __attribute__((packed))PS_MAPHEADER_OBJ, *PS_MAPHEADER_HANDLE;

/////////////////////////////////////////////////////////////////////////////////
// PES流头结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_PESHEADER
{
	unsigned short m1               ;             //mark
	unsigned char  pscp             ;             //packet_start_code_prefix
	unsigned char  si               ;			  //stream_id
	unsigned short ppl              ;             //PES_packet_length
	#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned short  ooc           :1;             //original_or_copy
	unsigned short  ct            :1;             //copyright
	unsigned short  dai           :1;             //data_alignment_indicator
	unsigned short  pp            :1;             //PES_priority
	unsigned short  psc           :2;			  //PES_scrambling_control
	unsigned short  ff            :2;             //fixed_field
	unsigned short  pef           :1;             //PES_extion_flag
	unsigned short  pcf           :1;             //PES_crc_flag
	unsigned short  acif          :1;             //additional_copy_info_flag
	unsigned short  dtmf          :1;             //DSM_trick_mode_flag
	unsigned short  erf           :1;             //ES_rate_flag
	unsigned short  ef            :1;             //ESCR_flag
	unsigned short  pdf           :2;             //PTS_DTS_flag
	#elif(BYTE_ORDER == BIG_ENDIAN)
	unsigned short  ff            :2;             //fixed_field
	unsigned short  psc           :2;			  //PES_scrambling_control
	unsigned short  pp            :1;             //PES_priority
	unsigned short  dai           :1;             //data_alignment_indicator
	unsigned short  ct            :1;             //copyright
	unsigned short  ooc           :1;             //original_or_copy
	unsigned short  pdf           :2;             //PTS_DTS_flag
	unsigned short  ef            :1;             //ESCR_flag
	unsigned short  erf           :1;             //ES_rate_flag
	unsigned short  dtmf          :1;             //DSM_trick_mode_flag
	unsigned short  acif          :1;             //additional_copy_info_flag
	unsigned short  pcf           :1;             //PES_crc_flag
	unsigned short  pef           :1;             //PES_extion_flag
	#endif
	unsigned char  phdl            ;              //PES_header_data_length
	unsigned char  ptsp            ;
	unsigned long  pts             ;              //PTS
	unsigned char  dtsp            ;
	unsigned long  dts             ;              //DTS
} __attribute__((packed)) PS_PESHEADER_OBJ, *PS_PESHEADER_HANDLE;
#endif
typedef struct _tagPS_PACKHEADER
{
	unsigned int   psc            ;		//pack_start_code
	unsigned short  scrb1          ;		//system_clock_reference_base1
	unsigned short  scrb2          ;		//system_clock_reference_base2
	unsigned short  scre           ;		//system_clock_reference_extension
	unsigned short  pmr            ;		//program_mux_rate
	unsigned char   mb             ;		//marker_bit
	#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned char   psl           :3;		//pack_stuffing_length
	unsigned char   res           :5;		//reserved
	#elif (BYTE_ORDER == BIG_ENDIAN)
	unsigned char   res           :5;		//reserved
	unsigned char   psl           :3;		//pack_stuffing_length
	#endif

} __attribute__((packed))PS_PACKHEADER_OBJ, *PS_PACKHEADER_HANDLE;


/////////////////////////////////////////////////////////////////////////////////
// PS流的System Header结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_SYSTEMHEADER
{
	unsigned int   shsc            ;           //system_header_start_code
	unsigned short  hl              ;           //head_length
	unsigned char   m1              ;           //fill marker_bit
	unsigned short  rbd             ;			//rate_bound
	unsigned char   uc1             ;
	unsigned char   uc2             ;
	unsigned char   uc3             ;
	unsigned char   vsi             ;           //video_stream_id
	unsigned short  vpbsb           ;           //video_P-STD_buffer_size_bound
	unsigned char   asi             ;           //aideo_stream_id
	unsigned short  apbsb           ;           //aideo_P-STD_buffer_size_bound	
} __attribute__((packed))PS_SYSTEMHEADER_OBJ, *PS_SYSTEMHEADER_HANDLE;

/////////////////////////////////////////////////////////////////////////////////
// PS流的Map Header结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_MAPHEADER
{
	unsigned char   pscp1           ;		//packet_start_code_prefix
	unsigned short  pscp            ;		//packet_start_code_prefix
	unsigned char   msi             ;		//map_stream_id
	unsigned short  psml            ;		//program_stream_map_length
	unsigned char   uc1             ;
	unsigned char   uc2             ;
	unsigned short  psil            ;       //program_stream_info_length
	unsigned short  esml            ;		//elementary_stream_map_length
	unsigned char   vst             ;		//video_stream_type	
	unsigned char   vesi            ;		//video_elementary_stream_id
	unsigned short  vesil           ;		//video_elementary_stream_info_length
	unsigned short  fill1           ;
	unsigned short  fill2           ;
	unsigned short  fill3           ;
	unsigned char   ast             ;		//aideo_stream_type	
	unsigned char   aesi            ;		//aideo_elementary_stream_id
	unsigned short  aesil           ;		//aideo_elementary_stream_info_length
	unsigned int   s32crc          ;		//32位校验和
} __attribute__((packed))PS_MAPHEADER_OBJ, *PS_MAPHEADER_HANDLE;

/////////////////////////////////////////////////////////////////////////////////
// PES流头结构体
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagPS_PESHEADER
{
	unsigned short m1               ;             //mark
	unsigned char  pscp             ;             //packet_start_code_prefix
	unsigned char  si               ;			  //stream_id
	unsigned short ppl              ;             //PES_packet_length
	#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned short  ooc           :1;             //original_or_copy
	unsigned short  ct            :1;             //copyright
	unsigned short  dai           :1;             //data_alignment_indicator
	unsigned short  pp            :1;             //PES_priority
	unsigned short  psc           :2;			  //PES_scrambling_control
	unsigned short  ff            :2;             //fixed_field
	unsigned short  pef           :1;             //PES_extion_flag
	unsigned short  pcf           :1;             //PES_crc_flag
	unsigned short  acif          :1;             //additional_copy_info_flag
	unsigned short  dtmf          :1;             //DSM_trick_mode_flag
	unsigned short  erf           :1;             //ES_rate_flag
	unsigned short  ef            :1;             //ESCR_flag
	unsigned short  pdf           :2;             //PTS_DTS_flag
	#elif(BYTE_ORDER == BIG_ENDIAN)
	unsigned short  ff            :2;             //fixed_field
	unsigned short  psc           :2;			  //PES_scrambling_control
	unsigned short  pp            :1;             //PES_priority
	unsigned short  dai           :1;             //data_alignment_indicator
	unsigned short  ct            :1;             //copyright
	unsigned short  ooc           :1;             //original_or_copy
	unsigned short  pdf           :2;             //PTS_DTS_flag
	unsigned short  ef            :1;             //ESCR_flag
	unsigned short  erf           :1;             //ES_rate_flag
	unsigned short  dtmf          :1;             //DSM_trick_mode_flag
	unsigned short  acif          :1;             //additional_copy_info_flag
	unsigned short  pcf           :1;             //PES_crc_flag
	unsigned short  pef           :1;             //PES_extion_flag
	#endif
	unsigned char  phdl            ;              //PES_header_data_length	
	unsigned char  ptsp            ;              
	unsigned int  pts             ;              //PTS
	unsigned char  dtsp            ;			  
	unsigned int  dts             ;              //DTS
} __attribute__((packed)) PS_PESHEADER_OBJ, *PS_PESHEADER_HANDLE;


static PS_FRAME_INFO_OBJ ps_info;

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_pes_video_init
// 描述：视频PES包初始化
// 参数：
//  ［IN］PS_PESHEADER_HANDLE p_pes_header - PES头结构体指针
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_pes_video_init(PS_PESHEADER_HANDLE p_pes_header)
{

	PESH_SET_M1(p_pes_header, 0);
	PESH_SET_PSCP(p_pes_header, 1);
	PESH_SET_SI(p_pes_header, VIDEO_STREAM);
	PESH_SET_FF(p_pes_header, 2);
	PESH_SET_PSC(p_pes_header, 0);
	PESH_SET_PP(p_pes_header, 0);
	PESH_SET_DAI(p_pes_header, 1);
	PESH_SET_CT(p_pes_header, 0);
	PESH_SET_OOC(p_pes_header, 0);
	PESH_SET_PDF(p_pes_header, PTS_DTS_FLAG);
	PESH_SET_EF(p_pes_header, 0);
	PESH_SET_ERF(p_pes_header, 0);
	PESH_SET_DTMF(p_pes_header, 0);
	PESH_SET_ACIF(p_pes_header, 0);
	PESH_SET_PCF(p_pes_header, 0);
	PESH_SET_PEF(p_pes_header, 0);
	PESH_SET_PHDL(p_pes_header, 10);

}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_pes_audio_init
// 描述：音频PES包初始化
// 参数：
//  ［IN］PS_PESHEADER_HANDLE p_pes_header - PES头结构体指针
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_pes_audio_init(PS_PESHEADER_HANDLE p_pes_header)
{

	PESH_SET_M1(p_pes_header, 0);
	PESH_SET_PSCP(p_pes_header, 1);
	PESH_SET_SI(p_pes_header, AUDIO_STREAM);
	PESH_SET_FF(p_pes_header, 2);
	PESH_SET_PSC(p_pes_header, 0);
	PESH_SET_PP(p_pes_header, 0);
	PESH_SET_DAI(p_pes_header, 1);
	PESH_SET_CT(p_pes_header, 0);
	PESH_SET_OOC(p_pes_header, 0);
	PESH_SET_PDF(p_pes_header, PTS_DTS_FLAG);
	PESH_SET_EF(p_pes_header, 0);
	PESH_SET_ERF(p_pes_header, 0);
	PESH_SET_DTMF(p_pes_header, 0);
	PESH_SET_ACIF(p_pes_header, 0);
	PESH_SET_PCF(p_pes_header, 0);
	PESH_SET_PEF(p_pes_header, 0);
	PESH_SET_PHDL(p_pes_header, 10);

}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_dts
// 描述：填充pes包中dts
// 参数：
//  ［OUT］ HB_U64 _ullTimeStamp - 输出时间戳
//   [IN] HB_U8* _ucpDtsP -
//   [IN] HB_U32* _pulDTS -
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_dts(HB_U64 _ullTimeStamp, HB_U8* _ucpDtsP, HB_U32* _pulDTS)
{
	HB_U64 ullTmp = _ullTimeStamp;
	*_ucpDtsP = 0;
	*_pulDTS = 0;
	
	//36~39
	(*_ucpDtsP) |= 1;
	
	//32~35
	(*_ucpDtsP) <<= 3;
	ullTmp >>= 30;
	ullTmp = ullTmp & 0x7;
	(*_ucpDtsP) |= ullTmp;
	(*_ucpDtsP) <<= 1;
	(*_ucpDtsP) |= MARKER_BIT;
	
	//16~31
	(*_pulDTS) <<= 15;
	ullTmp = _ullTimeStamp;
	ullTmp = (ullTmp >> 15) & 0x7FFF ;
	(*_pulDTS) |= ullTmp;
	(*_pulDTS) <<= 1;
	(*_pulDTS) |= MARKER_BIT;
	
	//0~15
	(*_pulDTS) <<= 15;
	ullTmp = _ullTimeStamp;
	ullTmp = ullTmp & 0x7FFF ;
	(*_pulDTS) |= ullTmp;
	(*_pulDTS) <<= 1;
	(*_pulDTS) |= MARKER_BIT;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_pts
// 描述：填充pes包中pts
// 参数：
//  ［OUT］ HB_U64 _ullTimeStamp - 输出时间戳
//   [IN] HB_U8* _ucpPtsP -
//   [IN] HB_U32* _pulPTS -
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_pts(HB_U64 _ullTimeStamp, HB_U8* _ucpPtsP, HB_U32* _pulPTS)
{
	HB_U64 ullTmp = _ullTimeStamp;
	*_ucpPtsP = 0;
	*_pulPTS = 0;
	
	//36~39
	(*_ucpPtsP) |= PTS_DTS_FLAG;
	
	//32~35
	(*_ucpPtsP) <<= 3;
	ullTmp >>= 30;
	ullTmp = ullTmp & 0x7;
	(*_ucpPtsP) |= ullTmp;
	(*_ucpPtsP) <<= 1;
	(*_ucpPtsP) |= MARKER_BIT;
	
	//16~31
	(*_pulPTS) <<= 15;
	ullTmp = _ullTimeStamp;
	ullTmp = (ullTmp >> 15) & 0x7FFF;
	(*_pulPTS) |= ullTmp;
	(*_pulPTS) <<= 1;
	(*_pulPTS) |= MARKER_BIT;
	
	//0~15
	(*_pulPTS) <<= 15;
	ullTmp = _ullTimeStamp;
	ullTmp = ullTmp & 0x7FFF ;
	(*_pulPTS) |= ullTmp;
	(*_pulPTS) <<= 1;
	(*_pulPTS) |= MARKER_BIT;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_packheader_make
// 描述：生成PS流的Pack Header
// 参数：
//  ［IN］ HB_U64 _ullTimeStamp - 时间戳
//   [IN] PS_PACKHEADER_HANDLE p_packheader - Pack Header的结构体指针
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_packheader_make(HB_U64 _ullTimeStamp, PS_PACKHEADER_HANDLE p_packheader)
{
	int i;
	HB_U64 ullTmp = _ullTimeStamp;
	HB_U16  usTmp = 0;
	PSPH_SET_PSC(p_packheader, htonl(0x01BA));

	usTmp |= 1;
	usTmp <<= 3;
	ullTmp >>= 30;
	ullTmp = ullTmp & 0x07;
	usTmp |= ullTmp;
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	ullTmp = _ullTimeStamp;
	ullTmp = (ullTmp >> 20) & 0x03FF;
	usTmp <<= 10;
	usTmp |= ullTmp;
	PSPH_SET_SCRB1(p_packheader, htons(usTmp));

	usTmp = 0;
	ullTmp = _ullTimeStamp;
	ullTmp = (ullTmp >> 15) & 0x1F;
	usTmp |= ullTmp;
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	ullTmp = _ullTimeStamp;
	ullTmp = (ullTmp >> 5) & 0x03FF;
	usTmp <<= 10;
	usTmp |= ullTmp;
	PSPH_SET_SCRB2(p_packheader, htons(usTmp));

	usTmp = 0;
	ullTmp = _ullTimeStamp;
	usTmp |= (ullTmp & 0x1F);
	usTmp <<= 1;
	usTmp |= MARKER_BIT;
	usTmp <<= 10;
	usTmp |= MARKER_BIT;
	PSPH_SET_SCRE(p_packheader, htons(usTmp));

	PSPH_SET_PMR(p_packheader, htons(0x0199));
	PSPH_SET_MB(p_packheader, 0x9F);
	PSPH_SET_RES(p_packheader, 0x1F);
	PSPH_SET_PSL(p_packheader, 0x00);
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_mapheader_make
// 描述：生成PS流的Map Header
// 参数：
//  ［IN］ HB_U64 _ullTimeStamp - 时间戳
//   [IN] PS_MAPHEADER_HANDLE p_mapheader - Map Header的结构体指针
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_mapheader_video_make(PS_MAPHEADER_HANDLE p_mapheader)
{
	PMSH_SET_PSCP1(p_mapheader, 0x00);
	PMSH_SET_PSCP(p_mapheader, htons(0x01));
	PMSH_SET_MSI(p_mapheader, 0xBC);
	PMSH_SET_PSML(p_mapheader, htons(0x18));
	PMSH_SET_UC1(p_mapheader, 0xE1);
	PMSH_SET_UC2(p_mapheader, 0xFF);
	PMSH_SET_PSIL(p_mapheader, htons(0x00));
	PMSH_SET_ESML(p_mapheader, htons(0x08));
	PMSH_SET_VST(p_mapheader, 0x1B);
	PMSH_SET_VESI(p_mapheader, VIDEO_STREAM);
	PMSH_SET_VESIL(p_mapheader, htons(0x06));
	PMSH_SET_AST(p_mapheader, 0x90);
	PMSH_SET_AESI(p_mapheader, AUDIO_STREAM);
	PMSH_SET_AESIL(p_mapheader, htons(0x00));
	PMSH_SET_CRC(p_mapheader, htonl(0x2EB90F3D));
}

#if 0
////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_mapheader_make
// 描述：生成PS流的Map Header
// 参数：
//  ［IN］ HB_U64 _ullTimeStamp - 时间戳
//   [IN] PS_MAPHEADER_HANDLE p_mapheader - Map Header的结构体指针
// 返回值：
//      无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_mapheader_audio_make(PS_MAPHEADER_HANDLE p_mapheader)
{
        PMSH_SET_PSCP1(p_mapheader, 0x00);
        PMSH_SET_PSCP(p_mapheader, htons(0x01));
        PMSH_SET_MSI(p_mapheader, 0xBC);
        PMSH_SET_PSML(p_mapheader, htons(0x18));
        PMSH_SET_UC1(p_mapheader, 0xE1);
        PMSH_SET_UC2(p_mapheader, 0xFF);
        PMSH_SET_PSIL(p_mapheader, htons(0x00));
        PMSH_SET_ESML(p_mapheader, htons(0x08));
        PMSH_SET_VST(p_mapheader, 0x90);   //g.711
        PMSH_SET_VESI(p_mapheader, VIDEO_STREAM);
        PMSH_SET_VESIL(p_mapheader, htons(0x06));
        PMSH_SET_AST(p_mapheader, 0x90);
        PMSH_SET_AESI(p_mapheader, AUDIO_STREAM);
        PMSH_SET_AESIL(p_mapheader, htons(0x00));
        PMSH_SET_CRC(p_mapheader, htonl(0x2EB90F3D));
}
#endif


////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_systemheader_make
// 描述：生成PS流的System Header
// 参数：
//   [IN] PS_SYSTEMHEADER_HANDLE p_systemheader - System Header的结构体指针
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_systemheader_make(PS_SYSTEMHEADER_HANDLE p_systemheader)
{
	PSSH_SET_SHSC(p_systemheader, htonl(0x01BB));
	PSSH_SET_HL(p_systemheader, htons(0x0C));
	PSSH_SET_M1(p_systemheader, 0x80);
	PSSH_SET_RBD(p_systemheader, htons(0xCCF5));
	PSSH_SET_UC1(p_systemheader, 0x04);
	PSSH_SET_UC2(p_systemheader, 0xE1);
	PSSH_SET_UC3(p_systemheader, 0x7F);
	PSSH_SET_VSI(p_systemheader, 0xE0);
	PSSH_SET_VPBSB(p_systemheader, htons(0xE0E8));
	PSSH_SET_ASI(p_systemheader, 0xC0);
	PSSH_SET_APBSB(p_systemheader, htons(0xC020));
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_pesheader_make
// 描述：生成PES头
// 参数：
//   [IN] PS_PESHEADER_HANDLE p_pesheader - PES Header的结构体指针
//  ［IN］ HB_S32 frame_len - 帧长度
// 返回值：
//  	无
// 说明：
////////////////////////////////////////////////////////////////////////////////
static HB_VOID ps_pesheader_make(PS_PESHEADER_HANDLE p_pesheader, HB_S32 frame_len, ES_FRAME_TYPE_E frame_type)
{
	HB_U32 ulTmp = 0;
	HB_U8 ucTmp = 0;
	HB_U32 ulPTS = 0;
	HB_U32 ulDTS = 0;
	HB_U32 ulFrameTime = 0;

	if (ES_FRAME_I == frame_type || ES_FRAME_P == frame_type)
	{
		//video
		//HB_U32 uiFrameNo = ps_info.vid_frame_number;
		ulFrameTime = 90000 / ps_info.framerate;
		//ulDTS = ulFrameTime * uiFrameNo;
		ulDTS = ps_info.vid_timestamp;
		ulPTS = ulDTS + ulFrameTime;

		ps_pts(ulPTS, &ucTmp, &ulTmp);
		PESH_SET_PTSP(p_pesheader, ucTmp);
		PESH_SET_PTS(p_pesheader, htonl(ulTmp));

		ps_dts(ulDTS, &ucTmp, &ulTmp);
		PESH_SET_DTSP(p_pesheader, ucTmp);
		PESH_SET_DTS(p_pesheader, htonl(ulTmp));

	}
	else if(ES_FRAME_AUDIO == frame_type)
	{
		ulDTS = ps_info.aud_timestamp;
		ulPTS = ps_info.aud_timestamp;
		ps_pts(ulPTS, &ucTmp, &ulTmp);
		PESH_SET_PTSP(p_pesheader, ucTmp);
		PESH_SET_PTS(p_pesheader, htonl(ulTmp));
		ps_dts(ulDTS, &ucTmp, &ulTmp);
		PESH_SET_DTSP(p_pesheader, ucTmp);
		PESH_SET_DTS(p_pesheader, htonl(ulTmp)); 
	}

	if(frame_len > MAX_PES_PACKET_LENGTH)
	{
		PESH_SET_PPL(p_pesheader, htons(0xFFFF));
	}
	else
	{
		PESH_SET_PPL(p_pesheader, htons(frame_len + PS_PES_PPL_LENGTH));
	}
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_init
// 描述：PS流初始化
// 参数：
//  ［IN］ HB_S32 framerate - 视频帧率
// 返回值：
//  	HB_FAILURE - 失败
//		HB_SUCCESS - 成功
// 说明：
////////////////////////////////////////////////////////////////////////////////
HB_S32 ps_init(HB_S32 framerate)
{
	memset(&ps_info, 0, sizeof(PS_FRAME_INFO_OBJ));
	ps_info.framerate = framerate;
	return HB_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_framerate_updata
// 描述：帧率刷新
// 参数：
//  ［IN］ HB_S32 framerate - 视频帧率
// 返回值：
//  	HB_FAILURE - 失败
//		HB_SUCCESS - 成功
// 说明：
////////////////////////////////////////////////////////////////////////////////
HB_S32 ps_framerate_updata(HB_S32 framerate)
{
	ps_info.framerate = framerate;
	return HB_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：ps_process
// 描述：ES流转换为PS流
// 参数：
//  ［IN］ HB_CHAR* p_srcbuf - ES流的地址指针
//   [IN] HB_S32 frame_len - ES流长度
//   [IN] ES_FRAME_TYPE_E frame_type - ES流帧类型
//   [OUT]PS_DATA_HANDLE p_ps_data - 输出PS流的结构体指针
// 返回值：
//  	HB_FAILURE - 失败
//		HB_SUCCESS - 成功
// 说明：
////////////////////////////////////////////////////////////////////////////////
//HB_S32 ps_process(HB_CHAR* p_srcbuf, HB_S32 frame_len, ES_FRAME_TYPE_E frame_type, PS_DATA_HANDLE p_ps_data)
HB_S32 ps_process(HB_CHAR* p_srcbuf, HB_S32 frame_len, ES_FRAME_TYPE_E frame_type, HB_CHAR* p_ps_data, HB_S32* p_ps_data_len)
{	
	HB_U32  uioffset = 0;
	HB_U32 uiTmpLen = 0;
	
//	HB_U32  uitimestamp = 0;
	HB_U32  uitimestamp = 90000 / ps_info.framerate * ps_info.vid_frame_number;
#if 0
	if (ps_info.vid_timestamp > ps_info.aud_timestamp)
	{
		if (ps_info.vid_timestamp - ps_info.aud_timestamp > 200000)
		{
			ps_info.aud_timestamp = ps_info.vid_timestamp;
		}
	}
	else if (ps_info.aud_timestamp > ps_info.vid_timestamp)
	{
		if (ps_info.aud_timestamp - ps_info.vid_timestamp > 200000)
		{
			ps_info.aud_timestamp = ps_info.vid_timestamp;
		}
	}
#endif
	if (ES_FRAME_I == frame_type || ES_FRAME_P == frame_type)
	{
//		uitimestamp = ps_info.vid_timestamp;

		//pack header
		//printf("pack video \n\n");

		PS_PACKHEADER_HANDLE p_packheader = (PS_PACKHEADER_HANDLE)(p_ps_data + uioffset);
		ps_packheader_make(uitimestamp, p_packheader);
		uioffset += sizeof(PS_PACKHEADER_OBJ);

		if(ES_FRAME_I == frame_type)
		{	
			//system header
			PS_SYSTEMHEADER_HANDLE p_systemheader = (PS_SYSTEMHEADER_HANDLE)(p_ps_data + uioffset);
			ps_systemheader_make(p_systemheader);
			uioffset += sizeof(PS_SYSTEMHEADER_OBJ);

			//map header
			PS_MAPHEADER_HANDLE p_mapheader = (PS_MAPHEADER_HANDLE)(p_ps_data + uioffset);
			ps_mapheader_video_make(p_mapheader);

			p_mapheader->fill1 = htons(0x0A04);
			p_mapheader->fill2 = htons(0x656E);
			p_mapheader->fill3 = htons(0x6700);

			uioffset += sizeof(PS_MAPHEADER_OBJ);
		}

		while(frame_len > 0)
		{   
			// pes header
			PS_PESHEADER_HANDLE p_pesheader = (PS_PESHEADER_HANDLE)(p_ps_data + uioffset);
			ps_pes_video_init(p_pesheader);
			ps_pesheader_make(p_pesheader, frame_len, frame_type);
			uioffset += sizeof(PS_PESHEADER_OBJ);

			// copy data
			if(frame_len > MAX_PES_PACKET_LENGTH)
			{
				uiTmpLen = MAX_PES_PACKET_LENGTH;
			}
			else
			{
				uiTmpLen = frame_len;
			}

			memcpy(p_ps_data + uioffset, p_srcbuf, uiTmpLen);

			frame_len -= uiTmpLen;
			p_srcbuf += uiTmpLen;
			uioffset += uiTmpLen;
		}
		ps_info.vid_frame_number++;
		ps_info.vid_timestamp += 90000/ps_info.framerate;
		*(p_ps_data_len) = uioffset;
	}

	else if (ES_FRAME_AUDIO == frame_type)
	{
		uitimestamp = ps_info.aud_timestamp;
		//TRACE_LOG("pack audio \n\n");
		//pack header

		#if 0
		PS_PACKHEADER_HANDLE p_packheader = (PS_PACKHEADER_HANDLE)(p_ps_data->ps_data + uioffset);
		ps_packheader_make(uitimestamp, p_packheader);
		uioffset += sizeof(PS_PACKHEADER_OBJ);
		//map header
		
		PS_MAPHEADER_HANDLE p_mapheader = (PS_MAPHEADER_HANDLE)(p_ps_data->ps_data + uioffset);
		ps_mapheader_audio_make(p_mapheader);
		

		p_mapheader->fill1 = htons(0x0A04);
		p_mapheader->fill2 = htons(0x656E);
		p_mapheader->fill3 = htons(0x6700);

		uioffset += sizeof(PS_MAPHEADER_OBJ);
		#endif
		while(frame_len > 0)
        {
        	uioffset = 0;
			
            // pes header
            PS_PESHEADER_HANDLE p_pesheader = (PS_PESHEADER_HANDLE)(p_ps_data + uioffset);
            ps_pes_audio_init(p_pesheader);
            ps_pesheader_make(p_pesheader, frame_len, frame_type);
            uioffset += sizeof(PS_PESHEADER_OBJ);

			// copy data
			if(frame_len > MAX_PES_PACKET_LENGTH)
			{
					uiTmpLen = MAX_PES_PACKET_LENGTH;
			}
			else
			{
					uiTmpLen = frame_len;
			}

			memcpy(p_ps_data + uioffset, p_srcbuf, uiTmpLen);

			frame_len -= uiTmpLen;
			p_srcbuf += uiTmpLen;
			uioffset += uiTmpLen;
        }

		ps_info.aud_frame_number++;
		//ps_info.aud_timestamp += 90000 / (1000 / 40);
		ps_info.aud_timestamp += 320;
		*p_ps_data_len = uioffset;

	}
	else	
	{
		return HB_FAILURE;
	}
	return HB_SUCCESS;
}

