#define _CRT_SECURE_NO_WARNINGS
#include "libcdeltaparser.h"

int main()
{
	//Instantiate the itemsdat struct from the header
	struct itemsdat itd;
	//Open file in read binary mode
	FILE* my_itemsdat = fopen("items.dat", "rb");
	//Pass file struct and itemsdat ptr to parse_itemsdat
	int resval = parse_itemsdat(my_itemsdat, &itd);
	
	//error check
	if (resval != 0)
	{
		printf("Error occured in parsing, error code: %d", resval);
		return 1;
	}
	//after this the itemsdat struct should be properly populated with all items.dat data, have fun
	return 0;
}
