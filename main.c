/*
 *  main.c
 *  AB2StarTAC
 *
 *  Created by John Thywissen on Sun Aug 24 2003.
 *  Copyright (c) 2003 John Thywissen. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <AddressBook/ABAddressBookC.h>
#include <stdio.h>
#include <time.h>

#define RELEASE(x) if (x != NULL) CFRelease(x); x = NULL;

int main (int argc, const char * argv[]) {
    const CFStringRef exportGroupName = CFStringCreateWithCString(NULL, "Export to StarTAC", NULL);
    const CFIndex initialPosition = 10L;
    const CFStringRef localPhonePrefixinAB = CFStringCreateWithCString(NULL, "+1", NULL);
    const CFStringRef localPhonePrefixinStarTAC = CFStringCreateWithCString(NULL, "", NULL);
    const CFStringRef intlPhonePrefixinAB = CFStringCreateWithCString(NULL, "+", NULL);
    const CFStringRef intlPhonePrefixinStarTAC = CFStringCreateWithCString(NULL, "011", NULL);
    const CFStringRef labelLookupKeys[] = {kABPhoneWorkLabel, kABPhoneHomeLabel, kABPhoneMobileLabel, kABPhoneMainLabel,  kABPhoneHomeFAXLabel, kABPhoneWorkFAXLabel, kABPhonePagerLabel};
    const char *labelLookupValues[] = {"office", "home", "mobile", "office", "fax", "fax", "pager"};
    CFDictionaryRef labelLookup = CFDictionaryCreate(NULL, labelLookupKeys, labelLookupValues, 7L, &kCFCopyStringDictionaryKeyCallBacks, NULL);

    ABAddressBookRef ab = ABGetSharedAddressBook();

    ABRecordRef me = (ABRecordRef)ABGetMe(ab);

    CFStringRef myfname = ABRecordCopyValue(me, kABFirstNameProperty);
    CFStringRef mylname = ABRecordCopyValue(me, kABLastNameProperty);
    time_t now = time(NULL);
    printf("#Export of %s %s's Mac OS X Address Book, \"%s\" group\n#Export date: %s#by AB2StarTAC v0.1, in StarTalk v0.4 format\n", CFStringGetCStringPtr(myfname, NULL), CFStringGetCStringPtr(mylname, NULL), CFStringGetCStringPtr(exportGroupName, NULL), ctime(&now));
    RELEASE(myfname);
    RELEASE(mylname);

#if 1
    ABSearchElementRef searchElement = ABGroupCreateSearchElement(kABGroupNameProperty, NULL, NULL, exportGroupName, kABEqualCaseInsensitive);
    CFArrayRef groups = ABCopyArrayOfMatchingRecords(ab, searchElement);
    RELEASE(searchElement);
    if (groups == NULL || CFArrayGetCount(groups) < 1) {
        fprintf(stderr,"#AB2StarTAC: Unable to find group \"%s\"\n", CFStringGetCStringPtr(exportGroupName, NULL));
        RELEASE(groups);
        exit(1);
    }
    CFArrayRef results = ABGroupCopyArrayOfAllMembers((ABGroupRef)CFArrayGetValueAtIndex(groups, 0));
    RELEASE(groups);
#else
    CFArrayRef results = ABCopyArrayOfAllPeople(ab);
#endif

    CFIndex count = CFArrayGetCount(results);
    printf("#Exported record count: %ld\n", count);
    CFIndex recindex;
    CFIndex position = initialPosition; /* Position in StarTAC phone book */
    for (recindex = 0L; recindex < count; recindex++)
    {
        ABRecordRef currRecord = (ABRecordRef)CFArrayGetValueAtIndex(results, recindex);

        CFStringRef line1 = ABRecordCopyValue(currRecord, kABLastNameProperty);
        CFStringRef line2 = ABRecordCopyValue(currRecord, kABFirstNameProperty);
        if (line1 == NULL) {
            line1 = ABRecordCopyValue(currRecord, kABOrganizationProperty);
        }

        ABMutableMultiValueRef multiValue = ABMultiValueCreateMutable();
        multiValue = (ABMutableMultiValueRef)ABRecordCopyValue(currRecord, kABPhoneProperty);
        unsigned int mvCount = ABMultiValueCount(multiValue);

        if (mvCount > 0 && line1 != NULL) {
            printf("\nposition: %ld\n", position++);

            printf("name: %s\n", CFStringGetCStringPtr(line1, NULL));

            if (line2 != NULL) {
                printf("company: %s\n", CFStringGetCStringPtr(line2, NULL));
            }

            ABMutableMultiValueRef multiValue = ABRecordCopyValue(currRecord, kABPhoneProperty);

            unsigned int mvCount = ABMultiValueCount(multiValue);
            int mvindex;
            for (mvindex = 0; mvindex < mvCount; mvindex++)
            {
                CFStringRef mvlabel = ABMultiValueCopyLabelAtIndex(multiValue, mvindex);
                CFStringRef text = ABMultiValueCopyValueAtIndex(multiValue, mvindex);
                CFMutableStringRef mvvalue = CFStringCreateMutableCopy(NULL, 0L, text);
                RELEASE(text);

                char *label = (char *)CFDictionaryGetValue(labelLookup, mvlabel);
                if (label == NULL) {
                    label = "qmark";
                }

                // Clean up phone numbers:
                // - Strip space - ( ) .
                CFRange range = CFRangeMake(0, CFStringGetLength(mvvalue));
                CFStringFindAndReplace(mvvalue, CFSTR(" "), CFSTR(""), range, 0);
                CFStringFindAndReplace(mvvalue, CFSTR("-"), CFSTR(""), range, 0);
                CFStringFindAndReplace(mvvalue, CFSTR("("), CFSTR(""), range, 0);
                CFStringFindAndReplace(mvvalue, CFSTR(")"), CFSTR(""), range, 0);
                CFStringFindAndReplace(mvvalue, CFSTR("."), CFSTR(""), range, 0);
                // - If leading +1, strip it
                range = CFRangeMake(0, CFStringGetLength(localPhonePrefixinAB));
                CFStringFindAndReplace(mvvalue, localPhonePrefixinAB, localPhonePrefixinStarTAC, range, 0);
                // - If other leading plus replace with 011
                range = CFRangeMake(0, CFStringGetLength(intlPhonePrefixinAB));
                CFStringFindAndReplace(mvvalue, intlPhonePrefixinAB, intlPhonePrefixinStarTAC, range, 0);
                // - TODO: DEBUG: complain if chars other than 0-9*#+o found

                printf("phone-%s: %s\n", label, CFStringGetCStringPtr(mvvalue, NULL));

                RELEASE(mvvalue);
                RELEASE(mvlabel);
            }
            RELEASE(multiValue);
        } else {
            printf("\n# Skipping a record with no phone numbers.\n");
        }
        multiValue = NULL;
        RELEASE(line1);
        RELEASE(line2);
    }
    RELEASE(results);

    return 0;
}
