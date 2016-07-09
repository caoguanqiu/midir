/*! \file httpc_file_download.h
 *  \brief 提供资源下载的接口
 *
 *  通过http client下载资源的接口 
 *
 */

#ifndef _HTTPC_FILE_DOWNLOAD_
#define _HTTPC_FILE_DOWNLOAD_ 

#include <stdint.h>

/** 处理http下载的data chunk的回调函数
 *
 *  http分片下载数据，每一个data chunk都单独交给该回调函数处理。
 *
 * @param[in] data http下载的数据片
 * @param[in] length 数据片的长度
 *
 * @return 0    表示数据处理成功，此后下载过程会继续
 * @return 负值  下载失败，http下载过程abort
 *  
 */
typedef int (*consume_chunked_data_t) (unsigned char *data,size_t length);

/** http response的教研函数
 *
 *  可以在其中校验response code, 资源的length等内容
 *
 * @param[in] resp http response的内容
 * 
 * @return true   校验成功，http下载过程会继续
 * @return false  下载失败，http过程abort
 *  
 */
typedef bool (*response_check_t)( http_resp_t *resp);

/** http client 下载接口
 *
 *  该接口为通用的http下载接口，可被用在需要http下载资源的场景。
 * 
 * @param[in] url_str 要下载的资源的URL
 * @param[in] checker http response 的检查函数，该函数通过返回值控制http的后续流程，
 *             返回true，http下载流程会继续，返回false,下载流程会被取消
 * @param[in] data_consumer 每一个http chunk数据包的处理函数
 *
 * @return 0    表示下载成功
 * @return 负值  下载失败
 *
 */
int httpc_download_file(const char * url_str,response_check_t checker,consume_chunked_data_t data_consumer);


#endif
