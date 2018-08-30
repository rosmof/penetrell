/************************************************

 *  Created on: Aug 26, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_FORM_DATA_H_
#define SRC_FORM_DATA_H_

/* NEXT_ELEMENTS is a special token that will be interpreted when parsing in get_form_fields function */
/* SPECIAL is a special token that will be interpreted when parsing in get_form_fields function */

static const char* formfields[] = {"SPECIAL_SCRIPT_MANAGER", "MSOWebPartPage_PostbackSource", "MSOTlPn_SelectedWpId",
        "MSOTlPn_View", "MSOTlPn_ShowSettings", "MSOGallery_SelectedLibrary", "MSOGallery_FilterString",
        "MSOTlPn_Button", "SPECIAL_NEXT_ELEMENTS", "__REQUESTDIGEST", "MSOSPWebPartManager_DisplayModeName",
        "MSOSPWebPartManager_ExitingDesignMode", "MSOWebPartPage_Shared", "MSOLayout_LayoutChanges",
        "MSOLayout_InDesignMode", "_wpSelected", "_wzSelected", "MSOSPWebPartManager_OldDisplayModeName",
        "MSOSPWebPartManager_StartWebPartEditingName", "MSOSPWebPartManager_EndWebPartEditing", "__VIEWSTATE",
        "__EVENTVALIDATION", "NULL_HARDCODED_GROUP", "SPECIAL_ASYNCPOST", nullptr};

// this fields are not in the returned GET request so they must be inserted by hand
// they all have null values
static const char* hardcoded_fields[] = {"InputKeywords",
        "ctl00$PlaceHolderMain$g_776bd304_3930_4f8c_9916_30d7d0aeebe6$SPTextSlicerValueTextControl",
        "ctl00$PlaceHolderMain$g_ee17c0e2_e12f_44e3_b9f1_f28fd2421582$SPTextSlicerValueTextControl",
        "ctl00$PlaceHolderMain$g_9461ba77_7d1f_4180_a65e_649a91f7f3d4$SPTextSlicerValueTextControl", "__spText1",
        "__spText2", "_wpcmWpid", "wpcmVal", nullptr};

static const char* formfield_start_fmt = "id=\x22%s\x22";

static const char* address_fmt = "http://portal.just.ro/%d/SitePages/Circumscriptii.aspx?id_inst=%d";

static const char separator_quotes = 0x22;
static const char separator_nextkey = 0x27;

static const char* script_manager_key = "ctl00$ScriptManager";
static const char* script_manager_value =
        "ctl00$PlaceHolderMain$g_17385422_131b_4c6c_89b4_9d3c87bc221a$updatePanelctl00_"
        "PlaceHolderMain_g_17385422_131b_4c6c_89b4_9d3c87bc221a|ctl00$PlaceHolderMain$g_"
        "17385422_131b_4c6c_89b4_9d3c87bc221a$ctl01";

static const char* PENETREL_HEADER_STATUS = "HTTP/1.1\0";
static const char* PENETREL_HEADER_LENGTH = "Content-Length:\0";

#define PENETREL_CURL_ERROR -1
#define PENETREL_HTTP_STATUS_OK 200

#endif
