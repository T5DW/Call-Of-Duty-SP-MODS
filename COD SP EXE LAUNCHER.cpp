#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <objbase.h>
#include <string>
#include <vector>
#include <io.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <unordered_map>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_RESTORE     1003
#define ID_TRAY_ALWAYSONTOP 1004
#define ID_TRAY_RANDOMQUOTE 1005
#define WM_SYSICON          (WM_USER + 1)

HBITMAP hBackground = nullptr;
HWND hComboGame = nullptr;
HWND hBtnPlay = nullptr;
HWND hBtnSetup = nullptr;
HWND hQuoteText = nullptr;
HWND hTitleText = nullptr;
HWND hCbufEdit = nullptr;
HWND hValueEdit = nullptr;
HWND hCbufBtn = nullptr;

HWND g_hWnd = nullptr;

NOTIFYICONDATAW nid = { 0 };
bool alwaysOnTop = false;

std::wstring t7mpGamePath;
std::wstring t5spGamePath;

HANDLE t5spProcessHandle = NULL;
DWORD t5spProcessId = 0;
uintptr_t moduleBase = 0;

// Your confirmed Cbuf_AddText offset
const uintptr_t CBUF_ADDTEXT_OFFSET = 0x00395BA8;

const wchar_t* quotes[] = {
    L"\"One shot, one kill.\"",
    L"\"Stay frosty.\"",
    L"\"All ghillied up and nowhere to go.\"",
    L"\"Remember... no Russian.\"",
    L"\"Good night, good luck.\"",
    L"\"Wyatt Stark is the best launcher maker ever!\""
};

int numQuotes = sizeof(quotes) / sizeof(quotes[0]);

std::unordered_map<std::wstring, std::wstring> t5Dvars = {
    {L"acousticSpikeMaxRange", L"200"},
    {L"acousticSpikeMinRadius", L"0.0001"},
    {L"acousticSpikeMinRange", L"0"},
    {L"acousticSpikeRingSize", L"20"},
    {L"acousticSpikeSize", L"10"},
    {L"ai_badPathSpam", L"0"},
    {L"ai_corpseCount", L"10"},
    {L"ai_debugAccuracy", L"0"},
    {L"ai_debugCoverSelection", L"0"},
    {L"ai_debugMeleeAttackSpots", L"0"},
    {L"ai_debugThreatSelection", L"0"},
    {L"ai_ShowCanSeeChecks", L"0"},
    {L"ai_ShowCanshootChecks", L"0"},
    {L"ai_showClaimedNode", L"0"},
    {L"ai_showRegion", L"0"},
    {L"animated_trees_enabled", L"0"},
    {L"bg_collectibles", L"0000000000000000000000000000000000000000000000000"},
    {L"cg_adsZoomToggleStyle", L"1"},
    {L"cg_adsZScaleMax", L"1.25"},
    {L"cg_animInfoCornerOffset", L"0 0"},
    {L"cg_blood", L"1"},
    {L"cg_bloodLimit", L"0"},
    {L"cg_bloodLimitMsec", L"330"},
    {L"cg_brass", L"1"},
    {L"cg_chatHeight", L"5"},
    {L"cg_chatTime", L"12000"},
    {L"cg_connectionIconSize", L"0"},
    {L"cg_crosshairAlphaMin", L"0.5"},
    {L"cg_crosshairDynamic", L"0"},
    {L"cg_crosshairEnemyColor", L"1"},
    {L"cg_debugInfoCornerOffset", L"0 0"},
    {L"cg_descriptiveText", L"1"},
    {L"cg_drawBreathHint", L"1"},
    {L"cg_drawCrosshair", L"1"},
    {L"cg_drawCrosshairNames", L"1"},
    {L"cg_drawFPS", L"Off"},
    {L"cg_drawFPSLabels", L"1"},
    {L"cg_drawFPSScale", L"0"},
    {L"cg_drawFriendlyFireCrosshair", L"0"},
    {L"cg_drawFriendlyNames", L"1"},
    {L"cg_drawLagometer", L"0"},
    {L"cg_drawMantleHint", L"1"},
    {L"cg_drawSnapshot", L"0"},
    {L"cg_drawSnapshotTime", L"1"},
    {L"cg_drawTurretCrosshair", L"1"},
    {L"cg_enemyNameFadeIn", L"250"},
    {L"cg_enemyNameFadeOut", L"250"},
    {L"cg_flareVisionSetFadeDuration", L"2000"},
    {L"cg_fov_default", L"65"},
    {L"cg_fov_default_thirdperson", L"40"},
    {L"cg_friendlyNameFadeIn", L"0"},
    {L"cg_friendlyNameFadeOut", L"1500"},
    {L"cg_gameBoldMessageWidth", L"390"},
    {L"cg_gameMessageWidth", L"455"},
    {L"cg_headIconMinScreenRadius", L"0.02"},
    {L"cg_hintFadeTime", L"100"},
    {L"cg_hudDamageIconHeight", L"64"},
    {L"cg_hudDamageIconInScope", L"0"},
    {L"cg_hudDamageIconOffset", L"128"},
    {L"cg_hudDamageIconTime", L"2000"},
    {L"cg_hudDamageIconWidth", L"128"},
    {L"cg_hudGrenadeIconEnabledFlash", L"0"},
    {L"cg_hudGrenadeIconHeight", L"25"},
    {L"cg_hudGrenadeIconInScope", L"1"},
    {L"cg_hudGrenadeIconMaxHeight", L"104"},
    {L"cg_hudGrenadeIconOffset", L"50"},
    {L"cg_hudGrenadeIconWidth", L"25"},
    {L"cg_hudGrenadePointerHeight", L"12"},
    {L"cg_hudGrenadePointerPivot", L"12 27"},
    {L"cg_hudGrenadePointerWidth", L"25"},
    {L"cg_hudMapBorderWidth", L"2"},
    {L"cg_hudMapFriendlyHeight", L"15"},
    {L"cg_hudMapFriendlyWidth", L"15"},
    {L"cg_hudMapPlayerHeight", L"20"},
    {L"cg_hudMapPlayerWidth", L"20"},
    {L"cg_hudMapRadarLineThickness", L"0.15"},
    {L"cg_hudProneY", L"-160"},
    {L"cg_hudStanceHintPrints", L"0"},
    {L"cg_invalidCmdHintBlinkInterval", L"600"},
    {L"cg_invalidCmdHintDuration", L"1800"},
    {L"cg_mapLocationSelectionCursorSpeed", L"0.6"},
    {L"cg_mapLocationSelectionRotationSpeed", L"3"},
    {L"cg_marks_ents_player_only", L"0"},
    {L"cg_marqueeTimeScale", L"30"},
    {L"cg_mature", L"1"},
    {L"cg_MaxDownedPulseRate", L"2"},
    {L"cg_MinDownedPulseRate", L"0.5"},
    {L"cg_motionblur_duration", L"2500"},
    {L"cg_motionblur_fadeout", L"500"},
    {L"cg_overheadIconSize", L"0.9"},
    {L"cg_overheadNamesFarDist", L"512"},
    {L"cg_overheadNamesFarScale", L"0.7"},
    {L"cg_overheadNamesFont", L"2"},
    {L"cg_overheadNamesGlow", L"0 0 0 1"},
    {L"cg_overheadNamesMaxDist", L"10000"},
    {L"cg_overheadNamesNearDist", L"64"},
    {L"cg_overheadNamesSize", L"0.8"},
    {L"cg_overheadRankSize", L"0.7"},
    {L"cg_playersInViewMinDot", L"0.98"},
    {L"cg_predictItems", L"1"},
    {L"cg_ScoresPing_BgColor", L"0.25098 0.25098 0.25098 0.501961"},
    {L"cg_ScoresPing_HighColor", L"0.8 0 0 1"},
    {L"cg_ScoresPing_Interval", L"100"},
    {L"cg_ScoresPing_LowColor", L"0 0.74902 0 1"},
    {L"cg_ScoresPing_MaxBars", L"4"},
    {L"cg_ScoresPing_MedColor", L"0.8 0.8 0 1"},
    {L"cg_scriptIconSize", L"0"},
    {L"cg_small_dev_string_fontscale", L"1"},
    {L"cg_sprintMeterDisabledColor", L"0.8 0.1 0.1 0.2"},
    {L"cg_sprintMeterEmptyColor", L"0.7 0.5 0.2 0.8"},
    {L"cg_sprintMeterFullColor", L"0.8 0.8 0.8 0.8"},
    {L"cg_subtitleMinTime", L"3"},
    {L"cg_subtitles", L"0"},
    {L"cg_subtitleWidthStandard", L"360"},
    {L"cg_subtitleWidthWidescreen", L"520"},
    {L"cg_teamChatsOnly", L"0"},
    {L"cg_viewZSmoothingMax", L"16"},
    {L"cg_viewZSmoothingMin", L"1"},
    {L"cg_viewZSmoothingTime", L"0.1"},
    {L"cg_visionSetLerpMaxDecreasePerFrame", L"0.01"},
    {L"cg_visionSetLerpMaxIncreasePerFrame", L"0.02"},
    {L"cg_voiceIconSize", L"0"},
    {L"cg_weaponCycleDelay", L"0"},
    {L"cg_youInKillCamSize", L"6"},
    {L"cl_allowDownload", L"0"},
    {L"cl_freelook", L"1"},
    {L"cl_maxpackets", L"30"},
    {L"cl_maxPing", L"800"},
    {L"cl_mouseAccel", L"0"},
    {L"cl_packetdup", L"1"},
    {L"cl_pitchspeed", L"140"},
    {L"cl_voice", L"1"},
    {L"cl_yawspeed", L"140"},
    {L"com_first_time_pc", L"0"},
    {L"com_maxfps", L"85"},
    {L"com_recommendedSet", L"1"},
    {L"com_startupIntroPlayed", L"1"},
    {L"compassClampIcons", L"1"},
    {L"compassCoords", L"740 3590 400"},
    {L"compassECoordCutoff", L"37"},
    {L"compassFriendlyHeight", L"10"},
    {L"compassFriendlyWidth", L"10"},
    {L"compassLocalRadarRadius", L"700"},
    {L"compassLocalRadarUpdateTime", L"2.25"},
    {L"compassMaxRange", L"3500"},
    {L"compassMinRadius", L"0.0001"},
    {L"compassMinRange", L"0.0001"},
    {L"compassObjectiveArrowHeight", L"20"},
    {L"compassObjectiveArrowOffset", L"2"},
    {L"compassObjectiveArrowRotateDist", L"5"},
    {L"compassObjectiveArrowWidth", L"20"},
    {L"compassObjectiveDrawLines", L"1"},
    {L"compassObjectiveHeight", L"20"},
    {L"compassObjectiveIconHeight", L"32"},
    {L"compassObjectiveIconHeightZombie", L"16"},
    {L"compassObjectiveIconWidth", L"32"},
    {L"compassObjectiveIconWidthZombie", L"16"},
    {L"compassObjectiveMaxRange", L"2048"},
    {L"compassObjectiveMinAlpha", L"1"},
    {L"compassObjectiveNumRings", L"10"},
    {L"compassObjectiveRingSize", L"80"},
    {L"compassObjectiveRingTime", L"10000"},
    {L"compassObjectiveTextHeight", L"18"},
    {L"compassObjectiveTextScale", L"0.3"},
    {L"compassObjectiveWidth", L"20"},
    {L"compassPartialType", L"0"},
    {L"compassPlayerHeight", L"25"},
    {L"compassPlayerWidth", L"25"},
    {L"compassRadarLineThickness", L"0.4"},
    {L"compassRadarPingFadeTime", L"4"},
    {L"compassRadarUpdateFastTime", L"2"},
    {L"compassRadarUpdateTime", L"4"},
    {L"compassRotation", L"1"},
    {L"compassSatellitePingFadeTime", L"10"},
    {L"compassSatelliteScanTime", L"1000"},
    {L"compassSatelliteStaticImageFadeTime", L"0"},
    {L"compassSize", L"1"},
    {L"compassSoundPingFadeTime", L"2"},
    {L"compassStaticImageUpdateTime", L"5000"},
    {L"compassTickertapeStretch", L"0.5"},
    {L"con_errormessagetime", L"8"},
    {L"con_gameMsgWindow0FadeInTime", L"0.25"},
    {L"con_gameMsgWindow0FadeOutTime", L"0.5"},
    {L"con_gameMsgWindow0Filter", L"gamenotify obituary"},
    {L"con_gameMsgWindow0LineCount", L"6"},
    {L"con_gameMsgWindow0MsgTime", L"5"},
    {L"con_gameMsgWindow0ScrollTime", L"0.25"},
    {L"con_gameMsgWindow0SplitscreenScale", L"1.5"},
    {L"con_gameMsgWindow1FadeInTime", L"0.25"},
    {L"con_gameMsgWindow1FadeOutTime", L"0.01"},
    {L"con_gameMsgWindow1Filter", L"boldgame"},
    {L"con_gameMsgWindow1LineCount", L"6"},
    {L"con_gameMsgWindow1MsgTime", L"8"},
    {L"con_gameMsgWindow1ScrollTime", L"0.25"},
    {L"con_gameMsgWindow1SplitscreenScale", L"1.5"},
    {L"con_gameMsgWindow2FadeInTime", L"0.75"},
    {L"con_gameMsgWindow2FadeOutTime", L"0.5"},
    {L"con_gameMsgWindow2Filter", L"subtitle"},
    {L"con_gameMsgWindow2LineCount", L"7"},
    {L"con_gameMsgWindow2MsgTime", L"5"},
    {L"con_gameMsgWindow2ScrollTime", L"0.25"},
    {L"con_gameMsgWindow2SplitscreenScale", L"1.5"},
    {L"con_gameMsgWindow3FadeInTime", L"0.25"},
    {L"con_gameMsgWindow3FadeOutTime", L"0.5"},
    {L"con_gameMsgWindow3Filter", L"coopinfo"},
    {L"con_gameMsgWindow3LineCount", L"6"},
    {L"con_gameMsgWindow3MsgTime", L"5"},
    {L"con_gameMsgWindow3ScrollTime", L"0.25"},
    {L"con_gameMsgWindow3SplitscreenScale", L"1.5"},
    {L"con_inputBoxColor", L"0.25 0.25 0.2 1"},
    {L"con_inputHintBoxColor", L"0.4 0.4 0.35 1"},
    {L"con_matchPrefixOnly", L"1"},
    {L"con_minicon", L"0"},
    {L"con_miniconlines", L"5"},
    {L"con_minicontime", L"4"},
    {L"con_outputBarColor", L"1 1 0.95 0.6"},
    {L"con_outputSliderColor", L"0.15 0.15 0.1 0.6"},
    {L"con_outputWindowColor", L"0.35 0.35 0.3 0.75"},
    {L"con_typewriterColorGlowCheckpoint", L"0.6 0.5 0.6 1"},
    {L"con_typewriterColorGlowCompleted", L"0 0.3 0.8 1"},
    {L"con_typewriterColorGlowFailed", L"0.8 0 0 1"},
    {L"con_typewriterColorGlowUpdated", L"0 0.6 0.18 1"},
    {L"con_typewriterDecayDuration", L"1000"},
    {L"con_typewriterDecayStartTime", L"4000"},
    {L"con_typewriterEnabledSounds", L"0"},
    {L"con_typewriterPrintSpeed", L"40"},
    {L"customclass1", L"CUSTOM 1"},
    {L"customclass2", L"CUSTOM 2"},
    {L"customclass3", L"CUSTOM 3"},
    {L"customclass4", L"CUSTOM 4"},
    {L"customclass5", L"CUSTOM 5"},
    {L"drew_notes", L"4"},
    {L"fx_marks", L"1"},
    {L"fx_marks_ents", L"1"},
    {L"fx_marks_smodels", L"1"},
    {L"g_allowvote", L"1"},
    {L"g_antilag", L"1"},
    {L"g_banIPs", L""},
    {L"g_clonePlayerMaxVelocity", L"80"},
    {L"g_deadChat", L"0"},
    {L"g_deathDelay", L"4000"},
    {L"g_dropForwardSpeed", L"10"},
    {L"g_dropHorzSpeedRand", L"100"},
    {L"g_dropUpSpeedBase", L"10"},
    {L"g_dropUpSpeedRand", L"5"},
    {L"g_log", L"games_mp.log"},
    {L"g_logSync", L"0"},
    {L"g_oldVoting", L"1"},
    {L"g_playerCollisionEjectSpeed", L"25"},
    {L"g_redCrosshairs", L"1"},
    {L"g_useGear", L"0"},
    {L"g_useholdspawndelay", L"1"},
    {L"g_voiceChatTalkingDuration", L"500"},
    {L"g_voteAbstainWeight", L"0.5"},
    {L"geographicalMatchmaking", L"0"},
    {L"gpad_buttonsConfig", L"buttons_default"},
    {L"gpad_enabled", L"0"},
    {L"gpad_menu_scroll_delay_first", L"420"},
    {L"gpad_menu_scroll_delay_rest", L"210"},
    {L"gpad_rumble", L"1"},
    {L"gpad_sticksConfig", L"thumbstick_default"},
    {L"hud_deathQuoteFadeTime", L"1000"},
    {L"hud_enable", L"1"},
    {L"hud_fade_ammodisplay", L"8"},
    {L"hud_fade_compass", L"8"},
    {L"hud_fade_healthbar", L"2"},
    {L"hud_fade_offhand", L"8"},
    {L"hud_fade_sprint", L"1.7"},
    {L"hud_fade_stance", L"1.7"},
    {L"hud_fadeout_speed", L"0.1"},
    {L"hud_flash_period_offhand", L"0.5"},
    {L"hud_flash_time_offhand", L"2"},
    {L"hud_health_pulserate_critical", L"0.5"},
    {L"hud_health_pulserate_injured", L"1"},
    {L"hud_health_startpulse_critical", L"0.33"},
    {L"hud_health_startpulse_injured", L"1"},
    {L"in_mouse", L"1"},
    {L"input_invertPitch", L"0"},
    {L"input_viewSensitivity", L"1"},
    {L"live_restrictEmblems", L"0"},
    {L"loc_forceEnglish", L"0"},
    {L"loc_language", L"0"},
    {L"log_append", L"0"},
    {L"m_filter", L"0"},
    {L"m_forward", L"0.25"},
    {L"m_pitch", L"0.022"},
    {L"m_side", L"0.25"},
    {L"m_yaw", L"0.022"},
    {L"monkeytoy", L"0"},
    {L"net_noudp", L"0"},
    {L"net_socksEnabled", L"0"},
    {L"net_socksPassword", L""},
    {L"net_socksPort", L"1080"},
    {L"net_socksServer", L""},
    {L"net_socksUsername", L""},
    {L"party_privacyStatus", L"2"},
    {L"prestigeclass1", L"CUSTOM 6"},
    {L"prestigeclass2", L"CUSTOM 7"},
    {L"prestigeclass3", L"CUSTOM 8"},
    {L"prestigeclass4", L"CUSTOM 9"},
    {L"prestigeclass5", L"CUSTOM 10"},
    {L"profile_physics", L"0"},
    {L"r_aaAlpha", L"dither (fast)"},
    {L"r_aaSamples", L"1"},
    {L"r_allow_intz", L"1"},
    {L"r_allow_null_rt", L"1"},
    {L"r_aspectRatio", L"auto"},
    {L"r_autopriority", L"0"},
    {L"r_backBufferSize", L"960"},
    {L"r_backBufferSizeY", L"544"},
    {L"r_blur_allowed", L"1"},
    {L"r_clipCodec", L"MJPEG"},
    {L"r_clipFPS", L"24"},
    {L"r_clipSize", L"360"},
    {L"r_customMode", L""},
    {L"r_debugLineWidth", L"1"},
    {L"r_depthPrepass", L"0"},
    {L"r_displayRefresh", L"60 Hz"},
    {L"r_distortion", L"1"},
    {L"r_dof_enable", L"1"},
    {L"r_fastSkin", L"0"},
    {L"r_filmLut", L"-1"},
    {L"r_flame_allowed", L"1"},
    {L"r_flameFX_distortionScaleFactor", L"0 1 1 0.511918"},
    {L"r_flameFX_enable", L"0"},
    {L"r_flameFX_fadeDuration", L"0.5"},
    {L"r_flameFX_FPS", L"15"},
    {L"r_flameFX_magnitude", L"0.0215147"},
    {L"r_fovScaleThresholdRigid", L"2.4"},
    {L"r_fovScaleThresholdSkinned", L"2.4"},
    {L"r_fullscreen", L"1"},
    {L"r_gamma", L"1"},
    {L"r_gfxopt_dynamic_foliage", L"1"},
    {L"r_gfxopt_water_simulation", L"1"},
    {L"r_glow_allowed", L"1"},
    {L"r_ignorehwgamma", L"0"},
    {L"r_inGameVideo", L"1"},
    {L"r_lodBiasRigid", L"0"},
    {L"r_lodBiasSkinned", L"0"},
    {L"r_lodScaleRigid", L"1"},
    {L"r_lodScaleSkinned", L"1"},
    {L"r_mode", L"800x600"},
    {L"r_monitor", L"0"},
    {L"r_motionblur_directionFactor", L"0.001"},
    {L"r_motionblur_enable", L"0"},
    {L"r_motionblur_frameBased_enable", L"0"},
    {L"r_motionblur_maxblur", L"30"},
    {L"r_motionblur_numberOfSamples", L"1"},
    {L"r_motionblur_positionFactor", L"0.01"},
    {L"r_multiGpu", L"1"},
    {L"r_multithreaded_device", L"0"},
    {L"r_picmip", L"0"},
    {L"r_picmip_bump", L"0"},
    {L"r_picmip_manual", L"0"},
    {L"r_picmip_spec", L"0"},
    {L"r_picmip_water", L"1"},
    {L"r_polygonOffsetBias", L"-1"},
    {L"r_polygonOffsetScale", L"-1"},
    {L"r_portalBevels", L"0.7"},
    {L"r_rendererPreference", L"Default"},
    {L"r_reviveFX_debug", L"0"},
    {L"r_reviveFX_fadeDuration", L"5"},
    {L"r_shaderWarming", L"0"},
    {L"r_specular", L"1"},
    {L"r_texFilterAnisoMax", L"16"},
    {L"r_texFilterAnisoMin", L"1"},
    {L"r_texFilterMipMode", L"Unchanged"},
    {L"r_use_driver_convergence", L"0"},
    {L"r_vsync", L"1"},
    {L"r_waterSheetingFX_allowed", L"1"},
    {L"r_waterSheetingFX_distortionScaleFactor", L"0.021961 1 0 0"},
    {L"r_waterSheetingFX_enable", L"0"},
    {L"r_waterSheetingFX_fadeDuration", L"2"},
    {L"r_waterSheetingFX_magnitude", L"0.0655388"},
    {L"r_waterSheetingFX_radius", L"4.44051"},
    {L"r_zfeather", L"1"},
    {L"ragdoll_enable", L"1"},
    {L"ragdoll_max_simulating", L"16"},
    {L"rate", L"25000"},
    {L"sensitivity", L"5"},
    {L"sm_enable", L"1"},
    {L"sm_maxLights", L"4"},
    {L"snaps", L"20"},
    {L"snd_enableEq", L"0"},
    {L"snd_khz", L"44"},
    {L"snd_losOcclusion", L"1"},
    {L"snd_menu_center_channel", L"1"},
    {L"snd_menu_cinematic", L"1"},
    {L"snd_menu_left_channel", L"1"},
    {L"snd_menu_left_surround", L"1"},
    {L"snd_menu_lfe", L"1"},
    {L"snd_menu_listen_level", L"0"},
    {L"snd_menu_master", L"0.8"},
    {L"snd_menu_music", L"1"},
    {L"snd_menu_right_channel", L"1"},
    {L"snd_menu_right_surround", L"1"},
    {L"snd_menu_sfx", L"1"},
    {L"snd_menu_speaker_setup", L"0"},
    {L"snd_menu_voice", L"1"},
    {L"snd_speakerConfiguration", L"Stereo"},
    {L"stats_backup", L"1"},
    {L"sv_allowDownload", L"1"},
    {L"sv_floodProtect", L"1"},
    {L"sv_HostBandwidthMinimumPerPlayer", L"65536"},
    {L"sv_hostname", L"BlackOpsZombie"},
    {L"sv_lastSaveCommitedToDevice", L""},
    {L"sv_lastSaveGame", L""},
    {L"sv_maxPing", L"0"},
    {L"sv_maxRate", L"7000"},
    {L"sv_minPing", L"0"},
    {L"sv_netcull_animcmdsize_threshold", L"2000"},
    {L"sv_netcull_msgsize_threshold", L"3000"},
    {L"sv_reconnectlimit", L"3"},
    {L"sv_useAnimCulling", L"0"},
    {L"sv_vac", L"1"},
    {L"sv_voice", L"1"},
    {L"sys_configSum", L"266827972"},
    {L"sys_configureGHz", L"0.0365968"},
    {L"sys_gpu", L"NVIDIA GeForce RTX 2060"},
    {L"sys_sysMB", L"1024"},
    {L"takeCoverWarnings", L"-1"},
    {L"team_indicator", L"0"},
    {L"ui_allowFov", L"0"},
    {L"ui_bigFont", L"0.4"},
    {L"ui_browserFriendlyfire", L"-1"},
    {L"ui_browserGameMode", L"0"},
    {L"ui_browserHideDlc", L"0"},
    {L"ui_browserKillcam", L"-1"},
    {L"ui_browserMod", L"-2"},
    {L"ui_browserShowDedicated", L"0"},
    {L"ui_browserShowEmpty", L"1"},
    {L"ui_browserShowFull", L"1"},
    {L"ui_browserShowPassword", L"-1"},
    {L"ui_browserShowPunkBuster", L"-1"},
    {L"ui_browserShowPure", L"0"},
    {L"ui_browserVoiceChat", L"-1"},
    {L"ui_currentMap", L"0"},
    {L"ui_currentNetMap", L"0"},
    {L"ui_dedicated", L"0"},
    {L"ui_drawCrosshair", L"1"},
    {L"ui_joinGametype", L"0"},
    {L"ui_netGametype", L"0"},
    {L"ui_serverStatusTimeOut", L"7000"},
    {L"ui_smallFont", L"0.25"},
    {L"ui_sp_unlock", L"0"},
    {L"useMapPreloading", L"1"},
    {L"useSvMapPreloading", L"1"},
    {L"vid_xpos", L"3"},
    {L"vid_ypos", L"22"},
    {L"voice_deadChat", L"0"},
    {L"voice_global", L"0"},
    {L"voice_localEcho", L"0"},
    {L"winvoice_mic_mute", L"1"},
    {L"winvoice_mic_reclevel", L"65535"},
    {L"winvoice_mic_scaler", L"1"},
    {L"winvoice_save_voice", L"0"},
    {L"zombiefive_discovered", L"1"},
    {L"zombietron_discovered", L"1"}
};

struct ModInfo {
    std::wstring name;
    std::wstring exeName;
    std::wstring status;
    bool enabled;
    bool requiresSetup;
    std::wstring currentPath;
};

std::vector<ModInfo> mods = {
    {L"T7MP Mod", L"blackops3.exe", L"(Enabled)", true, true, L""},
    {L"T5SP Mod", L"BlackOps.exe", L"(Enabled)", true, true, L""},
    {L"IW4SP Mod", L"iw4sp.exe", L"(Enabled)", true, false, L""},
    {L"T6MP Mod", L"t6mp.exe", L"(Coming Soon)", false, false, L""},
    {L"IW5MP Mod", L"iw5mp.exe", L"(Coming Soon)", false, false, L""}
};

bool FileExists(const std::wstring& path) {
    return _waccess_s(path.c_str(), 0) == 0;
}

void ShowRandomQuote() {
    int idx = rand() % numQuotes;
    SetWindowTextW(hQuoteText, quotes[idx]);
    PlaySoundW(L"click.wav", NULL, SND_ASYNC | SND_FILENAME);
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnapshot, &modEntry)) {
            do {
                if (_wcsicmp(modEntry.szModule, modName) == 0) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &modEntry));
        }
        CloseHandle(hSnapshot);
    }
    return modBaseAddr;
}

void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_SYSICON;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Wyatt's COD Launcher");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Restore");
    AppendMenuW(hMenu, MF_STRING | (alwaysOnTop ? MF_CHECKED : 0), ID_TRAY_ALWAYSONTOP, L"Always on Top");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RANDOMQUOTE, L"Random COD Quote");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

std::wstring BrowseForFolder(HWND hwnd, const wchar_t* title) {
    std::wstring result;
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        pfd->GetOptions(&dwOptions);
        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
        pfd->SetTitle(title);
        if (SUCCEEDED(pfd->Show(hwnd))) {
            IShellItem* psi = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    result = pszPath;
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

// Fixed order: UpdateButtonsAndSetup defined BEFORE UpdateGameCombo
void UpdateButtonsAndSetup() {
    int sel = (int)SendMessage(hComboGame, CB_GETCURSEL, 0, 0);
    bool modSelected = (sel != CB_ERR);
    bool modEnabled = modSelected && mods[sel].enabled;

    EnableWindow(hBtnPlay, modEnabled ? TRUE : FALSE);

    bool showSetup = modSelected && mods[sel].requiresSetup;
    ShowWindow(hBtnSetup, showSetup ? SW_SHOW : SW_HIDE);
    EnableWindow(hBtnSetup, showSetup ? TRUE : FALSE);

    bool isT5SP = modSelected && mods[sel].name == L"T5SP Mod";
    bool t5spRunning = isT5SP && t5spProcessHandle != NULL;
    ShowWindow(hCbufEdit, t5spRunning ? SW_SHOW : SW_HIDE);
    ShowWindow(hValueEdit, t5spRunning ? SW_SHOW : SW_HIDE);
    ShowWindow(hCbufBtn, t5spRunning ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(g_hWnd, 106), t5spRunning ? SW_SHOW : SW_HIDE);
}

void UpdateGameCombo() {
    SendMessage(hComboGame, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < mods.size(); ++i) {
        const auto& mod = mods[i];
        std::wstring text = mod.name;
        if (mod.requiresSetup && mod.currentPath.empty()) {
            text += L" (SET UP)";
        }
        else {
            text += L" " + mod.status;
        }
        SendMessage(hComboGame, CB_ADDSTRING, 0, (LPARAM)text.c_str());
    }
    SendMessage(hComboGame, CB_SETCURSEL, 0, 0);
    UpdateButtonsAndSetup();
}

void SetupMod(HWND hwnd, int index) {
    const auto& mod = mods[index];
    std::wstring title = L"Select folder for " + mod.name + L" (must contain " + mod.exeName + L")";
    std::wstring folder = BrowseForFolder(hwnd, title.c_str());
    if (folder.empty()) return;

    std::wstring fullExe = folder + L"\\" + mod.exeName;
    if (_waccess_s(fullExe.c_str(), 0) == 0) {
        mods[index].currentPath = folder;
        if (mod.name == L"T7MP Mod") t7mpGamePath = folder;
        if (mod.name == L"T5SP Mod") t5spGamePath = folder;
        MessageBoxW(hwnd, (mod.name + L" folder set successfully!").c_str(), L"Setup Complete", MB_OK | MB_ICONINFORMATION);
        UpdateGameCombo();
    }
    else {
        MessageBoxW(hwnd, (mod.exeName + L" not found in this folder!").c_str(), L"Invalid Folder", MB_OK | MB_ICONERROR);
    }
}

// Full Cbuf_AddText memory injection
bool InjectCbufCommand(const std::wstring& command) {
    if (t5spProcessHandle == NULL || moduleBase == 0) return false;

    uintptr_t cbufAddr = moduleBase + CBUF_ADDTEXT_OFFSET;

    std::string cmdA = std::string(command.begin(), command.end()) + "\n";
    SIZE_T size = cmdA.size() + 1;

    LPVOID alloc = VirtualAllocEx(t5spProcessHandle, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!alloc) return false;

    if (!WriteProcessMemory(t5spProcessHandle, alloc, cmdA.c_str(), size, NULL)) {
        VirtualFreeEx(t5spProcessHandle, alloc, 0, MEM_RELEASE);
        return false;
    }

    BYTE shellcode[] = {
        0x6A, 0x00,                          // push 0
        0x68, 0x00, 0x00, 0x00, 0x00,        // push alloc
        0xB8, 0x00, 0x00, 0x00, 0x00,        // mov eax, cbufAddr
        0xFF, 0xD0,                          // call eax
        0xC3                                 // ret
    };

    *(uintptr_t*)(shellcode + 3) = (uintptr_t)alloc;
    *(uintptr_t*)(shellcode + 8) = cbufAddr;

    LPVOID shellAlloc = VirtualAllocEx(t5spProcessHandle, NULL, sizeof(shellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!shellAlloc) {
        VirtualFreeEx(t5spProcessHandle, alloc, 0, MEM_RELEASE);
        return false;
    }

    if (!WriteProcessMemory(t5spProcessHandle, shellAlloc, shellcode, sizeof(shellcode), NULL)) {
        VirtualFreeEx(t5spProcessHandle, shellAlloc, 0, MEM_RELEASE);
        VirtualFreeEx(t5spProcessHandle, alloc, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(t5spProcessHandle, NULL, 0, (LPTHREAD_START_ROUTINE)shellAlloc, NULL, 0, NULL);
    if (!thread) {
        VirtualFreeEx(t5spProcessHandle, shellAlloc, 0, MEM_RELEASE);
        VirtualFreeEx(t5spProcessHandle, alloc, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(thread, 3000);
    CloseHandle(thread);

    VirtualFreeEx(t5spProcessHandle, shellAlloc, 0, MEM_RELEASE);
    VirtualFreeEx(t5spProcessHandle, alloc, 0, MEM_RELEASE);
    return true;
}

void LaunchSelected() {
    int sel = (int)SendMessage(hComboGame, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return;
    const auto& mod = mods[sel];

    if (!mod.enabled) {
        MessageBoxW(NULL, L"This mod is not ready yet.", L"Not Available", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (mod.requiresSetup && mod.currentPath.empty()) {
        MessageBoxW(NULL, L"Please set up the folder first!", L"Not Set Up", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring fullPath = mod.requiresSetup ? mod.currentPath + L"\\" + mod.exeName : mod.exeName;
    std::wstring workingDir = mod.requiresSetup ? mod.currentPath : L"";
    std::wstring args = L"";

    if (mod.name == L"T5SP Mod") {
        args = L"+set developer 1 +set sv_cheats 1 +set thereisacow 1337 +set con_errormessagetime 30 +set ui_allow_controls 1 +cg_drawVersion 1 +cg_drawVersionX 10 +cg_drawVersionY 10";
        t5spGamePath = mod.currentPath;
    }
    else if (mod.name == L"IW4SP Mod") {
        args = L"+set developer 1 +set thereisacow 1337 +set com_introPlayed 1";
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(fullPath.c_str(), args.empty() ? NULL : (LPWSTR)args.c_str(), NULL, NULL, FALSE, 0, NULL,
        workingDir.empty() ? NULL : workingDir.c_str(), &si, &pi)) {
        if (mod.name == L"T5SP Mod") {
            if (t5spProcessHandle) CloseHandle(t5spProcessHandle);
            t5spProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.dwProcessId);
            moduleBase = GetModuleBaseAddress(pi.dwProcessId, L"BlackOps.exe");
            CloseHandle(pi.hThread);
        }
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        ShowRandomQuote();
        UpdateButtonsAndSetup();
    }
    else {
        MessageBoxW(NULL, L"Failed to launch game!", L"Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(20, 20, 30)));

        hTitleText = CreateWindowW(L"STATIC", L"Wyatt Stark's COD Launcher", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 30, 900, 80, hwnd, NULL, NULL, NULL);
        HFONT hTitleFont = CreateFontW(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Impact");
        SendMessage(hTitleText, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

        hComboGame = CreateWindowW(WC_COMBOBOX, NULL,
            CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
            200, 200, 500, 600, hwnd, (HMENU)101, NULL, NULL);

        HFONT hComboFont = CreateFontW(36, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Arial");
        SendMessage(hComboGame, WM_SETFONT, (WPARAM)hComboFont, TRUE);

        hBtnSetup = CreateWindowW(L"BUTTON", L"SETUP FOLDER", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 320, 300, 60, hwnd, (HMENU)103, NULL, NULL);
        SendMessage(hBtnSetup, WM_SETFONT, (WPARAM)hComboFont, TRUE);
        ShowWindow(hBtnSetup, SW_HIDE);

        hBtnPlay = CreateWindowW(L"BUTTON", L"PLAY", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
            300, 450, 300, 120, hwnd, (HMENU)102, NULL, NULL);
        HFONT hPlayFont = CreateFontW(60, 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Impact");
        SendMessage(hBtnPlay, WM_SETFONT, (WPARAM)hPlayFont, TRUE);

        // Cbuf Tool
        CreateWindowW(L"STATIC", L"Dvar Command:", WS_VISIBLE | WS_CHILD,
            50, 580, 150, 30, hwnd, NULL, NULL, NULL);
        hCbufEdit = CreateWindowW(WC_EDIT, L"god", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            200, 580, 250, 30, hwnd, (HMENU)104, NULL, NULL);
        SendMessage(hCbufEdit, WM_SETFONT, (WPARAM)CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Arial"), TRUE);
        ShowWindow(hCbufEdit, SW_HIDE);

        CreateWindowW(L"STATIC", L"Value:", WS_VISIBLE | WS_CHILD,
            470, 580, 80, 30, hwnd, (HMENU)106, NULL, NULL);
        ShowWindow(GetDlgItem(hwnd, 106), SW_HIDE);

        hValueEdit = CreateWindowW(WC_EDIT, L"1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            550, 580, 150, 30, hwnd, (HMENU)107, NULL, NULL);
        SendMessage(hValueEdit, WM_SETFONT, (WPARAM)CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Arial"), TRUE);
        ShowWindow(hValueEdit, SW_HIDE);

        hCbufBtn = CreateWindowW(L"BUTTON", L"LIVE INJECT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            710, 580, 130, 30, hwnd, (HMENU)105, NULL, NULL);
        SendMessage(hCbufBtn, WM_SETFONT, (WPARAM)CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Arial"), TRUE);
        ShowWindow(hCbufBtn, SW_HIDE);

        hQuoteText = CreateWindowW(L"STATIC", L"Select a mod and click PLAY", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 620, 900, 50, hwnd, NULL, NULL, NULL);
        SendMessage(hQuoteText, WM_SETFONT, (WPARAM)hComboFont, TRUE);

        hBackground = (HBITMAP)LoadImageW(NULL, L"background.jpg", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        srand(GetTickCount());

        AddTrayIcon(hwnd);
        UpdateGameCombo();
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(200, 255, 200));
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0, 255, 100));
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == hComboGame) {
            UpdateButtonsAndSetup();
        }
        switch (LOWORD(wParam)) {
        case 102: LaunchSelected(); break;
        case 103: {
            int sel = (int)SendMessage(hComboGame, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && mods[sel].requiresSetup) {
                SetupMod(hwnd, sel);
            }
            break;
        }
        case 105: { // LIVE INJECT
            wchar_t dvarBuf[256];
            wchar_t valueBuf[256];
            GetWindowTextW(hCbufEdit, dvarBuf, 256);
            GetWindowTextW(hValueEdit, valueBuf, 256);
            std::wstring dvar = dvarBuf;
            std::wstring value = valueBuf;

            if (dvar.empty()) {
                MessageBoxW(NULL, L"Enter a dvar!", L"Error", MB_OK);
                break;
            }

            if (value.empty() && t5Dvars.count(dvar)) value = t5Dvars[dvar];

            if (value.empty()) {
                MessageBoxW(NULL, L"Enter a value!", L"Error", MB_OK);
                break;
            }

            std::wstring cmd = dvar + L" " + value;

            bool success = InjectCbufCommand(cmd);
            if (success) {
                MessageBoxW(NULL, (L"Live Cbuf_AddText injection success!\nCommand: " + cmd).c_str(),
                    L"Injected!", MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBoxW(NULL, L"Live injection failed (wrong offset?).", L"Error", MB_OK | MB_ICONERROR);
            }

            // Save to autoexec
            if (!t5spGamePath.empty()) {
                std::wstring cfgPath = t5spGamePath + L"\\players\\autoexec.cfg";
                std::wofstream cfg(cfgPath, std::ios::app);
                if (cfg.is_open()) {
                    cfg << L"seta " << dvar << L" " << value << L"\n";
                    cfg.close();
                }
            }
            break;
        }
        case ID_TRAY_RESTORE: ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); break;
        case ID_TRAY_EXIT:
            if (MessageBoxW(hwnd, L"Are you sure you want to exit?", L"Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
                DestroyWindow(hwnd);
            break;
        case ID_TRAY_ALWAYSONTOP:
            alwaysOnTop = !alwaysOnTop;
            SetWindowPos(hwnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            break;
        case ID_TRAY_RANDOMQUOTE:
            ShowRandomQuote();
            break;
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (hBackground) {
            HDC memDC = CreateCompatibleDC(hdc);
            SelectObject(memDC, hBackground);
            BITMAP bm;
            GetObject(hBackground, sizeof(bm), &bm);
            StretchBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            DeleteDC(memDC);
        }
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        if (t5spProcessHandle) CloseHandle(t5spProcessHandle);
        if (hBackground) DeleteObject(hBackground);
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    InitCommonControls();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WyattLauncher";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"WyattLauncher", L"Wyatt Stark's COD Launcher",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 920, 720, NULL, NULL, hInstance, NULL);

    g_hWnd = hwnd;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FreeConsole();
    CoUninitialize();
    return 0;
}