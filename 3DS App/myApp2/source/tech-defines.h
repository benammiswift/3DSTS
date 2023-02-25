//Defines for stuff that isn't UI shit

#define IP_ADDRESS_MAX_SIZE 15
#define IP_ADDRESS_ARR_SIZE (IP_ADDRESS_MAX_SIZE + 1)



//Networking

#define PORT 			6942
#define BSD_SUCCESS   	0
#define BSD_ERROR   	-1
#define BUFSIZE			16386

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
static u32 *SOC_buffer = NULL;

#define SPRING_TO_CENTRE_SENSIT 0.05
#define MAX_FLOAT_RANGE_EQUAL 0.02

#define VALUES_ARENT_WITHIN( x, y, z)  (fabs(x - y) > z) 
#define VALUES_ARE_WITHIN( x, y, z)  (fabs(x - y) < z)
#define VALUES_WITHIN_NOT_EQ( x, y, z)  ((fabs(x - y) < z) && (fabs(x - y) > 0.01)) 


#define CV_LIST_BLOCK_DELIM "??"
#define CV_LIST_ELEM_DELIM  "::"

#define COMM_SYNTAX_CVLIST          "CV_LISTING"
#define COMM_SYNTAX_LOCONAME        "LOCONAME"
#define COMM_SYNTAX_REQ_CVLIST      "REQ_CV_LIST"
//#define COMM_SYNTAX_TERM_MSG        "//END//"
#define COMM_SYNTAX_TERM_MSG        "**"
#define COMM_SYNTAX_REQ_LOCONAME    "REQ_LOCO"
#define COMM_SYNTAX_SETCV           "SET_CV"