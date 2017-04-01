#ifndef _WXH_RTP_OVER_RTSP_H_
#define _WXH_RTP_OVER_RTSP_H_

#include <stdint.h>

// h264 to rtp
#define MAX_RTP_PKT_LENGTH     1400  
#define PAYLOAD_TYPE_H264                    96 

typedef struct   
{  
    /**//* byte 0 */  
    unsigned char csrc_len:4;        /**//* expect 0 */  
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */  
    unsigned char padding:1;        /**//* expect 0 */  
    unsigned char version:2;        /**//* expect 2 */  
    /**//* byte 1 */  
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */  
    unsigned char marker:1;        /**//* expect 1 */  
    /**//* bytes 2, 3 */  
    unsigned short seq_no;
    /**//* bytes 4-7 */  
    unsigned  long timestamp;          
    /**//* bytes 8-11 */  
    unsigned long ssrc;            /**//* stream number is used here. */  
} RTP_FIXED_HEADER;  

typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char NRI:2;  
    unsigned char F:1;      
} NALU_HEADER; /**//* 1 BYTES */  

typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char NRI:2;   
    unsigned char F:1;      
} FU_INDICATOR; /**//* 1 BYTES */  

typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char R:1;  
    unsigned char E:1;  
    unsigned char S:1;      
} FU_HEADER; /**//* 1 BYTES */  


void get_rtsp_rtp_video_total_len(const uint8_t* pbuffer, uint32_t dwbuflen, uint32_t& ntotallen, uint32_t& nrtpnums)
{

    ntotallen = 0;
    nrtpnums = 0;
    uint8_t* pdata = (uint8_t*)pbuffer+5;
    uint8_t* pend = (uint8_t*)pbuffer+dwbuflen-9;
    while (pdata < pend)
    {
        int nnallen = (pdata[0]<<24) | (pdata[1]<<16) | (pdata[2]<<8) | (pdata[3]);
        pdata += 4;

        if (nnallen <= MAX_RTP_PKT_LENGTH)
        {
            ntotallen += 12 + nnallen;
            nrtpnums++;
        }
        else
        {
            int k = 0, last = 0;  
            k = nnallen / MAX_RTP_PKT_LENGTH;//��Ҫk��1400�ֽڵ�RTP��������Ϊʲô����1�أ���Ϊ�Ǵ�0��ʼ�����ġ�  
            last = nnallen% MAX_RTP_PKT_LENGTH;//���һ��RTP������Ҫװ�ص��ֽ���    
            nrtpnums += (k+1);
            ntotallen +=   (k+1)*14 + k*1400 + last -1;            
        }
        pdata += nnallen;
    }
    // 4���ֽ�ͷ��when rtp over rtsp
    ntotallen += 4*nrtpnums;
}

void set_buf_rtp_video_header(uint8_t* pbuffer, uint32_t dwssrc, uint32_t dwtimestample, uint32_t dwseqnum, bool marker)
{
    RTP_FIXED_HEADER* rtp_hdr = (RTP_FIXED_HEADER*)pbuffer;
    rtp_hdr->payload     = PAYLOAD_TYPE_H264;  //�������ͺţ�  
    rtp_hdr->version     = 2;  //�汾�ţ��˰汾�̶�Ϊ2              
    rtp_hdr->ssrc        = htonl(dwssrc);  
    rtp_hdr->timestamp=htonl(dwtimestample);
    //����rtp M λ��  
    rtp_hdr->marker = marker;  
    rtp_hdr->seq_no  = htons(dwseqnum); //���кţ�ÿ����һ��RTP����1��htons���������ֽ���ת�������ֽ���  
}
void set_buf_rtp_over_rtsp_tag(uint8_t* pbuffer, uint8_t bychannel, uint16_t dwlen)
{
    pbuffer[0] = 0x24;
    pbuffer[1] = bychannel;
    pbuffer[2] = (dwlen >> 8)&0xff;
    pbuffer[3] = dwlen&0xff;
}

bool generate_rtp_info_over_rtsp(const uint8_t* pbuffer, uint32_t dwbuflen, 
                                 uint8_t* pdest, uint32_t dwdestlen,  uint32_t dwtimestample, uint32_t& seq_num)
{
    memset(pdest, 0, dwdestlen);

    uint8_t* pdata = (uint8_t*)pbuffer +5;
    uint8_t* pend = (uint8_t*)pbuffer+dwbuflen-9;
    uint32_t dwdestinx = 0;
    RTP_FIXED_HEADER* rtp_hdr; 
    NALU_HEADER* nalu_hdr;  
    FU_INDICATOR* fu_ind;  
    FU_HEADER* fu_hdr;  
    uint32_t dwssrc = 0x10;
    while (pdata < pend)
    {
        int nnallen = (pdata[0]<<24) | (pdata[1]<<16) | (pdata[2]<<8) | (pdata[3]);
        pdata += 4;

        uint8_t bynaltype = pdata[0];
        if (nnallen <= MAX_RTP_PKT_LENGTH)
        {
            set_buf_rtp_over_rtsp_tag(&pdest[dwdestinx], 0, 12+nnallen);
            dwdestinx += 4;

            set_buf_rtp_video_header(&pdest[dwdestinx], dwssrc, dwtimestample, seq_num++, true);
            //����NALU HEADER,�������HEADER����sendbuf[12]  
            nalu_hdr =(NALU_HEADER*)&pdest[dwdestinx+12]; //��sendbuf[12]�ĵ�ַ����nalu_hdr��֮���nalu_hdr��д��ͽ�д��sendbuf�У�          

            nalu_hdr->F = (bynaltype & 0x80)>>7;  
            nalu_hdr->NRI = (bynaltype & 0x60)>>5;  //��Ч������n->nal_reference_idc�ĵ�6��7λ����Ҫ����5λ���ܽ���ֵ����nalu_hdr->NRI��  
            nalu_hdr->TYPE = (bynaltype & 0x1f) ;  


            memcpy(&pdest[dwdestinx+13], &pdata[1], nnallen-1);
            dwdestinx += 12 + nnallen;
        }
        else
        {
            //�õ���nalu��Ҫ�ö��ٳ���Ϊ1400�ֽڵ�RTP��������  
            int k = 0, last = 0;  
            k = nnallen / 1400;//��Ҫk��1400�ֽڵ�RTP��������Ϊʲô����1�أ���Ϊ�Ǵ�0��ʼ�����ġ�  
            last = nnallen % 1400;//���һ��RTP������Ҫװ�ص��ֽ���
            int t = 0;//����ָʾ��ǰ���͵��ǵڼ�����ƬRTP��


            while(t <= k)  
            {                
                if(!t)//����һ����Ҫ��Ƭ��NALU�ĵ�һ����Ƭ����FU HEADER��Sλ,t = 0ʱ������߼���  
                {
                    set_buf_rtp_over_rtsp_tag(&pdest[dwdestinx], 0, 14+1400);
                    dwdestinx += 4;

                    set_buf_rtp_video_header(&pdest[dwdestinx], dwssrc, dwtimestample, seq_num++, false);

                    //����FU INDICATOR,�������HEADER����sendbuf[12]  
                    fu_ind =(FU_INDICATOR*)&pdest[dwdestinx+12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�  
                    fu_ind->F = (bynaltype & 0x80)>>7;
                    fu_ind->NRI = (bynaltype & 0x60)>>5;
                    fu_ind->TYPE = 28;  //FU-A���͡�  

                    //����FU HEADER,�������HEADER����sendbuf[13]  
                    fu_hdr =(FU_HEADER*)&pdest[dwdestinx+13];  
                    fu_hdr->E = 0;  
                    fu_hdr->R = 0;  
                    fu_hdr->S = 1;  
                    fu_hdr->TYPE = (bynaltype & 0x1f);  

                    memcpy(&pdest[dwdestinx+14], &pdata[1], 1400);
                    dwdestinx += 14 + 1400;

                    t++;  
                }  
                //����һ����Ҫ��Ƭ��NALU�ķǵ�һ����Ƭ������FU HEADER��Sλ������÷�Ƭ�Ǹ�NALU�����һ����Ƭ����FU HEADER��Eλ  
                else if(k == t)//���͵������һ����Ƭ��ע�����һ����Ƭ�ĳ��ȿ��ܳ���1400�ֽڣ��� l> 1386ʱ����  
                {
                    set_buf_rtp_over_rtsp_tag(&pdest[dwdestinx], 0, (14 + last-1));
                    dwdestinx += 4;

                    //����rtp M λ����ǰ����������һ����Ƭʱ��λ��1  
                    set_buf_rtp_video_header(&pdest[dwdestinx], dwssrc, dwtimestample, seq_num++, true);

                    //����FU INDICATOR,�������HEADER����sendbuf[12]  
                    fu_ind =(FU_INDICATOR*)&pdest[dwdestinx+12]; 
                    fu_ind->F= (bynaltype & 0x80)>>7;
                    fu_ind->NRI=(bynaltype & 0x60)>>5;
                    fu_ind->TYPE=28;  

                    //����FU HEADER,�������HEADER����sendbuf[13]  
                    fu_hdr = (FU_HEADER*)&pdest[dwdestinx+13];  
                    fu_hdr->R = 0;  
                    fu_hdr->S = 0;  
                    fu_hdr->TYPE = (bynaltype & 0x1f); 
                    fu_hdr->E = 1;  

                    //��nalu���ʣ���l-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����  
                    memcpy(&pdest[dwdestinx+14], &pdata[ t*1400 + 1], last-1);                                        
                    dwdestinx += 14 + last-1;

                    t++;                  
                }  
                //�Ȳ��ǵ�һ����Ƭ��Ҳ�������һ����Ƭ�Ĵ���  
                else if(t < k && 0 != t)  
                {  
                    set_buf_rtp_over_rtsp_tag(&pdest[dwdestinx], 0, 14 + 1400);
                    dwdestinx += 4;

                    set_buf_rtp_video_header(&pdest[dwdestinx], dwssrc, dwtimestample, seq_num++, false);
                    //����FU INDICATOR,�������HEADER����sendbuf[12]  
                    fu_ind = (FU_INDICATOR*)&pdest[dwdestinx+12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�  
                    fu_ind->F = (bynaltype & 0x80)>>7;
                    fu_ind->NRI = (bynaltype & 0x60)>>5;
                    fu_ind->TYPE = 28;  

                    //����FU HEADER,�������HEADER����sendbuf[13]  
                    fu_hdr =(FU_HEADER*)&pdest[dwdestinx+13];  
                    fu_hdr->R = 0;  
                    fu_hdr->S = 0;  
                    fu_hdr->E = 0;  
                    fu_hdr->TYPE = (bynaltype & 0x1f); 

                    memcpy(&pdest[dwdestinx+14], &pdata[t * 1400 + 1], 1400);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����  
                    dwdestinx += 14 + 1400;
                    t++;  
                }  
            }  
        }

        pdata += nnallen;
    }
    return dwdestlen == dwdestinx;
}

#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string.hpp>  
#include <iostream>

std::string get_base_rtsp_url(std::string& strurl)
{
	std::string baseurl;
	std::vector<std::string> item;
	boost::split_regex( item, strurl, boost::regex( "\\?" ) );             
	if (item.size() > 2)
	{
		return strurl;
	}
	baseurl = item[0];
	if (baseurl.back() != '/')
	{
		baseurl += '/';
	}
	return baseurl;
}
int get_url_track_num(std::string& strual)
{
	int track = 0;
	if (strual.back() == '1')
	{
		track = 1;
	}
	else if (strual.back() == '2')
	{
		track = 2;
	}
	return track;
}

// all the queryid and the option will in map ,  the request operator is get by 'command'
bool get_all_options_from_text(const std::string& strrequest, std::map< std::string, std::string >& mapitems)
{
    mapitems.clear();
    std::vector<std::string> veclines;
    boost::split_regex( veclines, strrequest, boost::regex( "\r\n" ) ); 
    bool byfirst = false;
    for(auto& strline : veclines)
    {
        if (strline.empty()) {continue;}

        if (false == byfirst)
        {
            byfirst = true;
            std::vector<std::string> item;
            boost::trim(strline);
            boost::split_regex( item, strline, boost::regex( "[ ]+" ) );             
            if (item.size() != 3)
            {
                return false;
            }
            mapitems["methond"] = item[0];
            mapitems["url"] = item[1];
			mapitems["baseurl"] = get_base_rtsp_url(item[1]);
            mapitems["protocol"] = item[2];
            
            boost::regex expression("([0-9a-zA-Z@.]+)=([0-9a-zA-Z@.]+)"); 
            boost::smatch what;
            std::string::const_iterator start = item[1].begin();
            std::string::const_iterator end = item[1].end();
            while( boost::regex_search(start, end, what, expression) )  
            {
                mapitems[what[1].str()] = what[2].str();
                start = what[0].second;
            }
        }
        else
        {
            std::vector<std::string> item;
            boost::trim(strline);
            boost::split_regex( item, strline, boost::regex( ":" ) ); 
            if (item.size() != 2)
            {
                return false;
            } 
            else
            {
                boost::trim(item[0]);
                boost::trim(item[1]);
                mapitems[item[0]] = item[1];
            }
        }
    }
	std::cout << "-----------------------------------------------------------" << std::endl;
    for(auto& mapitems:mapitems)
    {		
        std::cout << mapitems.first << ":" << mapitems.second << std::endl;
    }
	return true;
}

#define MAX_RTSP_TCP_BUFFER_LEN_  32*1024
bool get_rtsp_message_from_buffer(boost::asio::streambuf& readstreambuf, std::string& strmessage)
{
    /*strmessage.clear();
    boost::asio::const_buffer buffer = readstreambuf.data();
    const char* pdatabuf = boost::asio::buffer_cast<char*>(buffer);
    int nbuflen = boost::asio::buffer_size(buffer);*/

    boost::asio::streambuf::const_buffers_type cbt = readstreambuf.data(); 
    std::string request_data(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt)); 
    int nbuflen = request_data.length(); 

    //should not be this, but from the buffer alloc this maybe right
        
    char* ptempbuffer = (char*)request_data.c_str();
    int nconsume = 0;
    
    while (nconsume < nbuflen)
    {
        if ('$' == ptempbuffer[nconsume])
        {
            int nleft = nbuflen-nconsume;
            if ( nleft < 4) break;
            
            uint8_t byhigh = ptempbuffer[nconsume+2];
            uint8_t bylow = ptempbuffer[nconsume+3];
            int nrtplen = byhigh << 8 | bylow;

            if(nleft < nrtplen+4) break;
            nconsume += nrtplen+4;
        }
        else
        {
            const char* pend = strstr(&ptempbuffer[nconsume], "\r\n\r\n");
            if (pend)
            {
                int nmsglen = pend - (&ptempbuffer[nconsume]) + 4;
                strmessage += std::string(&ptempbuffer[nconsume], nmsglen);
                nconsume += nmsglen;
            }
            else
            {
                break;
            }            
        }
    }
    if (nconsume > 0) readstreambuf.consume(nconsume);    
    if (strmessage.length() > 0) return true;
    
    if (0 == nconsume && nbuflen > MAX_RTSP_TCP_BUFFER_LEN_)
    {
        std::cout << "the buffer is bigger than 32k, still no rtsp or rtp get, whar r u doing, clear all xxxxxx\r\n";
        readstreambuf.consume(nbuflen);        
    }    
    return false;
}

#endif