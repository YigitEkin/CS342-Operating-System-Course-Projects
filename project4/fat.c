#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <linux/posix_types.h>
#include <linux/types.h>
#include <asm/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/msdos_fs.h>
#include <ctype.h>
#include <math.h>
#define SECTORSIZE  512 
#define CLUSTERSIZE 1024

/*
int main() {
    //use substr to split a string into two substrings
    char * str = "Hello World Arda";
    char * s = str;
    while (*s != '\0') 
    {
        //printf("%s\n", s);
        int count = indexOf(s, ' ');
        if (count == -1)
        {
            char * substr = substring(s, strlen(s));
            s += strlen(s);
            printf("%s\n", substr);
        }
        
        char * substr = substring(s, count);
        s += count + 1;
        printf("%s\n", substr);
    }
    
    return 0;
}
*/

char * substring(char * str, int end) {
    int len = strlen(str);
    int start = 0;
    int i = 0;
    char * substr = malloc(sizeof(char) * (end - start + 1));
    while (i < end) {
        substr[i] = str[i];
        i++;
    }
    substr[i] = '\0';
    return substr;
}

// write a function that finds the index of a chracter in a string
// the function should take a string and a character
// and return the index of the character in the string
int indexOf(char * str, char c) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == c) {
            return i;
        }
        i++;
    }
    return -1;
}

void tokenize(char* str, char* c, int count, char** arr) {
    char * s = str;
    int index = 0;
    while ( index < count && *s != '\0') 
    {
        int count = indexOf(s, c);
        if (count == -1)
        {
            char * substr = substring(s, strlen(s));
            s += strlen(s);
            printf("%s\n", substr);
            arr[index++] = substr;
        }
        
        char * substr = substring(s, count);
        s += count + 1;
        printf("%s\n", substr);

        arr[index++] = substr;
    }
}

int calculateFileCountInPath(char * path) {
     int count = 0;
     
  	for(int i = 0; i < strlen(path); i++)
     {
  		if(path[i] == '/')  
		{
  			count++;
 		}
	}

     return count;
}
 
void toUpperCase(char* str) {
     char*s = str;
     while(*s) {
          *s = toupper((unsigned char) *s);
          s++;
     }    
     str = s; 
}

void printUnSignedString( unsigned char * str) {
     int length = strlen(str);
     for (size_t i = 0; i < length; i++)
     {
          if (isprint(str[i]))
          {
               printf("%c",str[i]);
          }
          
     }
}
int readsector (int fd, unsigned char *buf,  
         unsigned int snum) 
{ 
off_t offset; 
  int n; 
  offset = snum * SECTORSIZE; 
  lseek (fd, offset, SEEK_SET); 
  
  n = read(fd, buf, SECTORSIZE); 

  if (n == SECTORSIZE) 
    return (0); 
  else 
    return(-1); 
}    
  
int readcluster (int fd, unsigned char *buf,  
       unsigned int cnum) 
{ 
off_t offset; 
int n; 
     unsigned int snum; // sector number 
     snum = 2064 +  
(cnum - 2)  * 2; 
offset = snum * SECTORSIZE;  
lseek (fd, offset, SEEK_SET); 
n = read (fd, buf, CLUSTERSIZE); 
if (n == CLUSTERSIZE) 
       return (0);   // success 
  else  
       return (-1);    
} 

void printSummaryInformation(char* diskimage) {
     struct fat_boot_sector *bp;
     struct fat_boot_fsinfo *bp2;
     unsigned char buf[512];
     int disk1 = open(diskimage, O_RDONLY);
     readsector(disk1,buf,0);
     bp = (struct fat_boot_sector *) buf;
     bp2 = (struct fat_boot_fsinfo *) buf;
     printf("File system type: ");
     printUnSignedString(&bp->fat32.fs_type);
     //printf("Volume Label: %s\n", bp->fat16.vol_label);
     printf("\n");
     printf("Volume Label:\n"); //TODO
     printf("number of sector in disk: %u\n", bp->total_sect);
     printf("sector size in bytes: %d\n", SECTOR_SIZE);
     printf("number of reserved sectors:%u\n", bp->reserved);
     printf("number of sectors per fat table: %u\n", bp->fat32.length);
     printf("number of fat tables: %u\n", bp->fats);
     printf("number of sectors per cluster: %u\n", bp->sec_per_clus);
     printf("number of clusters:\n");
     printf("data region starts at sector: %u\n", bp->fats * bp->fat32.length + bp->reserved);
     printf("root directory starts at sector: %u\n", bp->fats * bp->fat32.length + bp->reserved);
     printf("root directory starts at cluster: %d\n", (bp->fats * bp->fat32.length + bp->reserved) / CLUSTERSIZE);
     printf("disk size in bytes %u bytes\n", bp->total_sect * SECTOR_SIZE);
     printf("disk size in megabytes %uMB\n", bp->total_sect * SECTOR_SIZE / 1048576 );
     printf("number of used clusters: %d\n");
     printf("number of free clusters: %d\n");
     
     //Volume label ?
     //number of cluster 1024 fazla
     //number of used clusters ?
     //number of free clusters ?
     close(disk1);
}

void printSectorContent(char* diskimage, int secNum) {
      unsigned char buf[SECTOR_SIZE];
     int disk1 = open(diskimage, O_RDONLY | O_SYNC, 0);
     readsector(disk1,buf,secNum);
     for (size_t i = 0; i < SECTOR_SIZE; i+=16)
     {
          printf("%08x: ", i);
          for (size_t j = 0; j < 16; j++)
          {
               if(j % 2 == 0 && j != 0 ) {
                    printf(" ");
               } 
                    printf("%02x ", buf[i+j]);
          }

          for(int z = 0; z <16; z++) {
               printf("%c", isprint(buf[z+i]) ? buf[i+z] : '.');
          }
          printf("\n");
     }
}

void printClusterContent(char* diskimage, int cnum) {
     unsigned char buf[CLUSTERSIZE];
     int disk1 = open(diskimage, O_RDONLY | O_SYNC, 0);
     readcluster(disk1,buf,cnum);
     for (size_t i = 0; i < CLUSTERSIZE; i+=16)
     {
          printf("%08x: ", i);
          for (size_t j = 0; j < 16; j++)
          {
               if(j % 2 == 0 && j != 0 ) {
                    printf(" ");
               } 
                    printf("%02x ", buf[i+j]);
          }

          for(int z = 0; z <16; z++) {
               printf("%c", isprint(buf[z+i]) ? buf[i+z] : '.');
          }
          printf("\n");
     }
}


void printFileSummary(char* diskimage, char* path) {
     unsigned char buf[CLUSTERSIZE];
     int disk1 = open(diskimage, O_RDONLY | O_SYNC, 0);
     readcluster(disk1,buf,2);
     int length = calculateFileCountInPath(path);
     char split[length];



     struct msdos_dir_entry * dep;

     dep = (struct msdos_dir_entry *) buf;
        
}

void help() {
printf("\n13) print  a  help  page  showing  all  12  options:./fat <DISKIMAGE> -h\n");  
printf("\n12) print a map of the volume for the first count clusters if count is -1 will print all clusters: \n/fat <DISKIMAGE> -m <COUNT>\n");  
printf("\n11)read COUNT bytes from the file  indicated  with  PATH  starting  at  OFFSET  (byte  offset  into  the  file)  and print  the  bytes  read  to  the  screen: \n ./fat <DISKIMAGE> -r PATH OFFSET COUNT\n");  
printf("\n10)prints the content of the FAT table. The first COUNT entries will be printed out. Each line will include a cluster number (FAT index in decimal form) and the respective FAT entry content in decimal form. Entry index will start from 0 If COUNT is -1, information about all entries will be printed out: \n./fat <DISKIMAGE> -f <COUNT>\n");

printf("\n9)prints the content of the directory entry of the file or directory indicated with PATH. Some information from the directory entry will be printed out:\n./fat <DISKIMAGE> -d <PATH> \n");

printf("\n8)pprint the numbers of the clusters storing the content of the file or directoryindicated with PATH. If the size of the file is 0, nothing will be printed. :\n./fat <DISKIMAGE> -n <PATH> \n");

printf("\n7)prints the names of the files and subdirectories in the directory indicated with PATH. In each line of output, the name of a file or directory (not pathname), its first cluster number, its size (in bytes), and its date-time information is printed:\n./fat <DISKIMAGE> -l <PATH> \n");

printf("\n6)prints the content (byte sequence) of the file indicated with PATH to the screen in hex form:\n./fat <DISKIMAGE> -b <PATH> \n");

printf("\n5)prints the content of the ascii text file indicated with PATH to the screen as it is:\n./fat <DISKIMAGE> -a <PATH> \n");

printf("\n4)prints all directories and their files and subdirectories starting from the root directory,recursively, in a depth-first search order:\n./fat <DISKIMAGE> -t\n");

printf("\n3):prints the content (byte sequence) of the specified cluster to the screen in hex form:\n./fat <DISKIMAGE> -c <cnum>\n");

printf("\n2)print the content (byte sequence) of the specified sector to screen in hex form.:\n./fat <DISKIMAGE> -s <SECTORNUM> \n");

printf("\n1)print some summary information about the specified FAT32 volume DISKIMAGE.:\n./fat <DISKIMAGE> -v \n");
}

void toggleEndian(char* str) {
     int len = strlen(str);
     printf("%d", len);
     char* temp;
     for (size_t i = 0; i < len-1; i++)
     {
          temp[i] = str[len-i-1];
     }
     strcpy(str,temp);
}

void printFAT(int count, char* diskimage) {
     unsigned char buf[CLUSTERSIZE];
     int disk1 = open(diskimage, O_RDONLY | O_SYNC, 0);
     int snum = count / 32 + 32;
     int offset = count % 32;
     int index = 0;
     int cnt = 0;
     readsector(disk1,buf,snum);
     while (index < snum)
     {
          for (size_t i = 0; i < SECTOR_SIZE; i+=16)
          {
               
               for (size_t j = 0; j < 16; j++)
               {
                    if ((i + j) % 4 == 0)
                    {
                       printf("\n%08x: ", cnt);
                       cnt++;
          
                    }
                       printf("%c", (buf[i+j]) );
               }
          }

          index++;
     }
     printf("\n");
}

int main(int argc, char*args[]) {
     //printFileSummary("disk", "/FILE1.BIN");
     //printFAT(0,"disk");
     

     char* c = "1234";
     //printf("a\n");

     toggleEndian(c);
     printf("%s", c);
     printf("af\n");

     printf("%s",c);
     
     
     //printFileSummary("disk", a);
     if (argc == 3) {
          if (strcmp(args[2], "-h") == 0) {
               help();
          } else if (strcmp(args[2], "-v") == 0) {
               printSummaryInformation(args[1]);
          } else if (strcmp(args[2], "-t") == 0) {
               //f4
          }
     } else if (argc == 6) {
          if (strcmp(args[2], "-r") == 0) {
               int count = atoi(args[5]);
               int offset = atoi(args[4]);
               char* path = args[3];
               //f11
          }
     } else if (argc == 4) {
          if (strcmp(args[2], "-s") == 0) {
               int sectornum = atoi(args[3]);
               printSectorContent(args[1],sectornum);
               //f2
          } else if (strcmp(args[2], "-c") == 0) {
               int clusternum = atoi(args[3]);
               printClusterContent(args[1],clusternum);
               //f3
          } else if (strcmp(args[2], "-a") == 0) {
               char* path = args[3];
               //f5
          } else if (strcmp(args[2], "-b") == 0) {
               char* path = args[3];
               //f6
          } else if (strcmp(args[2], "-l") == 0) {
               char* path = args[3];
               //f7
          } else if (strcmp(args[2], "-n") == 0) {
               char* path = args[3];
               //f8
          } else if (strcmp(args[2], "-d") == 0) {
               char* path = args[3];
               //f9
          } else if (strcmp(args[2], "-f") == 0) {
               int count = atoi(args[3]);
               //f10
          } else if (strcmp(args[2], "-m") == 0) {
               int count = atoi(args[3]);
               //f12
          }
     }
     return 0;
}