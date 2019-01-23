#ifndef PACKET_H
#define PACKET_H

#define GET_CHAR(pack)    *(char*)pack;   pack+=1;
#define GET_SHORT(pack)   *(short*)pack;  pack+=sizeof(short);
#define GET_INT(pack)     *(int*)pack;    pack+=sizeof(int);
#define GET_FLOAT(pack)   *(float*)pack;  pack+=sizeof(float);
#define GET_DOUBLE(pack)  *(double*)pack; pack+=sizeof(double);
#define GET_LONGLONG(pack)  *(long long*)pack; pack+=sizeof(long long);
#define GET_LEN(pack)     GET_INT(pack)
#define GET_TYPE(pack)    GET_CHAR(pack)
#define GET_VAR_STRING(pack,buf,bufSize)  strncpy(buf,pack,bufSize);  pack+=strlen(pack)+1;
#define GET_FIX_STRING(pack,buf,maxSize)  strncpy(buf,pack,maxSize);  pack+=maxSize;
#define GET_BYTES(pack,buf,getlen)        memcpy(buf,pack,getlen); pack += getlen;

#define SET_CHAR(pack,v)    *(char*)pack=v;   pack+=1;
#define SET_SHORT(pack,v)   *(short*)pack=v;  pack+=sizeof(short);
#define SET_INT(pack,v)     *(int*)pack=v;    pack+=sizeof(int);
#define SET_FLOAT(pack,v)   *(float*)pack=v;  pack+=sizeof(float);
#define SET_DOUBLE(pack,v)  *(double*)pack=v; pack+=sizeof(double);
#define SET_LONGLONG(pack,v)  *(long long*)pack=v; pack+=sizeof(long long);
#define SET_TYPE(pack,v)      SET_CHAR(pack,v);
#define SET_FIX_STRING(pack,v,maxSize)    strncpy(pack,v,maxSize);pack+=maxSize;
#define SET_VAR_STRING(pack,v,maxSize)    strncpy(pack,v,maxSize);pack+=strlen(v)+1;
#define SET_BYTES(pack,v,len)  memcpy(pack,v,len);pack+=len;
#define OFFSET(pack,v)    pack+=v;

#endif // PACKET_H
