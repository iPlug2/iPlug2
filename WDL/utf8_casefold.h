#ifndef _WDL_UTF8_CASEFOLD_H_
#define _WDL_UTF8_CASEFOLD_H_

/*
 * Extreme case-folding, we fold latin characters with accents down to a-z if possible
 * e.g.
 *   00C0; C; 00E0; # LATIN CAPITAL LETTER A WITH GRAVE
 * goes to 'a'
 *
 * but
 *   0402; C; 0452; # CYRILLIC CAPITAL LETTER DJE
 *
 * goes to U+0452
 *
 * you can run php utf8_casefold.h > new_version.h to regenerate this
 *
 *


<?php
$fp = fopen("CaseFolding.txt","r"); // https://www.unicode.org/Public/UCD/latest/ucd/CaseFolding.txt
$adjmap = [];
$revmap = [];
$names = [];

function add($b1, $b2, $nb1, $nb2) {
  global $adjmap, $revmap;
  if (!isset($adjmap[$b1])) $adjmap[$b1] = [];
  $offs = $nb1 < 0x80 || $nb2 == 0 ? "$nb1" : ("$nb1 " . ($nb2 - $b2));
  if (!isset($adjmap[$b1][$offs])) $adjmap[$b1][$offs] = [];
  $adjmap[$b1][$offs][] = $b2;

  if ($nb1 != $b1) {
    if (!isset($revmap[$nb1])) $revmap[$nb1] = [];
    $revmap[$nb1][] = $b1;
  }
}

function getcpn($b1, $b2) {
  global $names;
  $cp = (($b1&0x1f)<<6) + ($b2&0x3f);
  $debug_names = false; // long long, option
  $n = $debug_names && isset($names[$cp]) ? " " . $names[$cp] : "";
  return sprintf("U+%04X%s",$cp,$n);
}

while ($x = fgets($fp))
{
  $a = sscanf($x, "%x; %c; %x");
  if (!isset($a[2])) continue;
  if ($a[1] == 'F' || $a[0] < 0x80 || $a[0] >= 0x800 || $a[2] >= 0x800) continue;
  $b2 = 0x80 + ($a[0]&0x3f);
  $b1 = 0xc0 + ($a[0]>>6);
  if (preg_match("@#\s*(.*)\s*$@",$x,$res)) $names[$a[0]] = $res[1];

  $nb1 = $a[2];
  $nb2 = 0;
  if ($nb1 >= 0x80) {
    $nb2 = 0x80 + ($nb1&0x3f);
    $nb1 = 0xc0 + ($nb1>>6);
    if (preg_match("#LATIN.*?LETTER ([A-Z])\s#",$x,$res)) {
      $m = ord($res[1]) + 0x20;
      add($nb1,$nb2,$m,0);
      $nb1 = $m;
    }
  }
  add($b1,$b2,$nb1,$nb2);
}
ksort($adjmap);
ksort($revmap);

fclose($fp);
$fp = fopen($argv[0],"r");
if (!$fp) die("error opening self\n");
$hadopen = false;
while ($x = fgets($fp))
{
  if (strstr($x,"<" . "?php")) $hadopen=true;
  if ($hadopen) echo $x;
  if (trim($x) == "// begin auto-generated") break;
}
fclose($fp);
?>

static WDL_STATICFUNC_UNUSED void wdl_utf8_2byte_casefold(unsigned char &byte1, unsigned char &byte2)
{
  // does not fold uppercase ASCII to lowercase, caller should do that if desired.
  switch (byte1)
  {
<?php

foreach ($adjmap as $b1 => $fb) {
  printf("    case 0x%x:\n",$b1);
  printf("      switch (byte2)\n      {\n");
  foreach ($fb as $offs => $list) {
    $cc=0;
    printf("       ");
    sort($list);
    $comment = "";
    $adj = sscanf($offs, "%d %d");
    foreach ($list as $b2) {
      if ($cc++ >= 6) { printf(" // $comment\n       "); $cc=0; $comment = ""; }
      printf(" case 0x%x:",$b2);
      $comment .= ($comment != "" ? ", " : "") . getcpn($b1,$b2);
      if (isset($adj[1]) && $adj[1] != 0) $comment .= " -> " . getcpn($adj[0],$b2 + $adj[1]);
    }
    if ($adj[0] != $b1) printf($adj[0] < 0x80 ? " byte1 = '%c';" : " byte1 = 0x%x;",$adj[0]);

    if (!isset($adj[1])) { printf(" byte2 = 0;"); $comment .= sprintf(" -> '%c'",$adj[0]); }
    else if ($adj[1] != 0 && count($list)==1) printf(" byte2 = 0x%x;",$list[0] + $adj[1]);
    else if ($adj[1] > 0) printf(" byte2 += 0x%x;",$adj[1]);
    else if ($adj[1] < 0) printf(" byte2 -= 0x%x;",-$adj[1]);
    echo " break;" . ($comment != "" ? " // $comment\n" : "\n");
  }
  printf("      }\n");
  printf("    break;\n");
}
?>
  }
}

static WDL_STATICFUNC_UNUSED char wdl_utf8_2byte_casefold1(unsigned char byte1, unsigned char byte2)
{
  wdl_utf8_2byte_casefold(byte1,byte2);
  return byte1;
}

static WDL_STATICFUNC_UNUSED const unsigned char *wdl_utf8_scan_unfolded(const unsigned char *src, unsigned char c) {
<?php
echo "  #define SCAN(exp) for(;;) { const unsigned char v = *src; if (!v) break; if (exp) return src; src++; }\n";
echo "  switch (c)\n  {\n";
foreach ($revmap as $db=>$list)
{
  sort($list);
  $list = array_unique($list);
  $body = "";
  foreach ($list as $k) $body .= sprintf("%sv == 0x%02x",$body=="" ? "" : " || ",$k);
  if (count($list)>2) $body = sprintf("v >= 0x%02x && (",$list[0]) . $body . ")";

  if ($db >= ord('a') && $db <= ord('z'))
    printf("    case '%c': SCAN((v|0x20) == c || ((%s) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;\n",$db,$body);
  else printf("    case 0x%x: SCAN(v == c || (%s)) break;\n",$db,$body);
}
echo "    default: if (c>='a'&&c<='z') SCAN((v|0x20) == c) else SCAN(v == c); break;\n";
echo "  }\n";
echo "  #undef SCAN\n";
echo "  return NULL;\n";
echo "}\n";
echo "\n#endif\n";
die;

?>

*/
// begin auto-generated

static WDL_STATICFUNC_UNUSED void wdl_utf8_2byte_casefold(unsigned char &byte1, unsigned char &byte2)
{
  // does not fold uppercase ASCII to lowercase, caller should do that if desired.
  switch (byte1)
  {
    case 0xc2:
      switch (byte2)
      {
        case 0xb5: byte1 = 0xce; byte2 = 0xbc; break; // U+00B5 -> U+03BC
      }
    break;
    case 0xc3:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: // U+00C0, U+00C1, U+00C2, U+00C3, U+00C4, U+00C5
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: byte1 = 'a'; byte2 = 0; break; // U+00E0, U+00E1, U+00E2, U+00E3, U+00E4, U+00E5 -> 'a'
        case 0x86: case 0x90: case 0x9e: byte2 += 0x20; break; // U+00C6 -> U+00E6, U+00D0 -> U+00F0, U+00DE -> U+00FE
        case 0x87: case 0xa7: byte1 = 'c'; byte2 = 0; break; // U+00C7, U+00E7 -> 'c'
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0xa8: case 0xa9: // U+00C8, U+00C9, U+00CA, U+00CB, U+00E8, U+00E9
        case 0xaa: case 0xab: byte1 = 'e'; byte2 = 0; break; // U+00EA, U+00EB -> 'e'
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0xac: case 0xad: // U+00CC, U+00CD, U+00CE, U+00CF, U+00EC, U+00ED
        case 0xae: case 0xaf: byte1 = 'i'; byte2 = 0; break; // U+00EE, U+00EF -> 'i'
        case 0x91: case 0xb1: byte1 = 'n'; byte2 = 0; break; // U+00D1, U+00F1 -> 'n'
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x98: // U+00D2, U+00D3, U+00D4, U+00D5, U+00D6, U+00D8
        case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb8: byte1 = 'o'; byte2 = 0; break; // U+00F2, U+00F3, U+00F4, U+00F5, U+00F6, U+00F8 -> 'o'
        case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0xb9: case 0xba: // U+00D9, U+00DA, U+00DB, U+00DC, U+00F9, U+00FA
        case 0xbb: case 0xbc: byte1 = 'u'; byte2 = 0; break; // U+00FB, U+00FC -> 'u'
        case 0x9d: case 0xbd: case 0xbf: byte1 = 'y'; byte2 = 0; break; // U+00DD, U+00FD, U+00FF -> 'y'
      }
    break;
    case 0xc4:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: byte1 = 'a'; byte2 = 0; break; // U+0100, U+0101, U+0102, U+0103, U+0104, U+0105 -> 'a'
        case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b: // U+0106, U+0107, U+0108, U+0109, U+010A, U+010B
        case 0x8c: case 0x8d: byte1 = 'c'; byte2 = 0; break; // U+010C, U+010D -> 'c'
        case 0x8e: case 0x8f: case 0x90: case 0x91: byte1 = 'd'; byte2 = 0; break; // U+010E, U+010F, U+0110, U+0111 -> 'd'
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: // U+0112, U+0113, U+0114, U+0115, U+0116, U+0117
        case 0x98: case 0x99: case 0x9a: case 0x9b: byte1 = 'e'; byte2 = 0; break; // U+0118, U+0119, U+011A, U+011B -> 'e'
        case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xa0: case 0xa1: // U+011C, U+011D, U+011E, U+011F, U+0120, U+0121
        case 0xa2: case 0xa3: byte1 = 'g'; byte2 = 0; break; // U+0122, U+0123 -> 'g'
        case 0xa4: case 0xa5: case 0xa6: case 0xa7: byte1 = 'h'; byte2 = 0; break; // U+0124, U+0125, U+0126, U+0127 -> 'h'
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: // U+0128, U+0129, U+012A, U+012B, U+012C, U+012D
        case 0xae: case 0xaf: case 0xb0: byte1 = 'i'; byte2 = 0; break; // U+012E, U+012F, U+0130 -> 'i'
        case 0xb2: byte2 = 0xb3; break; // U+0132 -> U+0133
        case 0xb4: case 0xb5: byte1 = 'j'; byte2 = 0; break; // U+0134, U+0135 -> 'j'
        case 0xb6: case 0xb7: byte1 = 'k'; byte2 = 0; break; // U+0136, U+0137 -> 'k'
        case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: // U+0139, U+013A, U+013B, U+013C, U+013D, U+013E
        case 0xbf: byte1 = 'l'; byte2 = 0; break; // U+013F -> 'l'
      }
    break;
    case 0xc5:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: byte1 = 'l'; byte2 = 0; break; // U+0140, U+0141, U+0142 -> 'l'
        case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: byte1 = 'n'; byte2 = 0; break; // U+0143, U+0144, U+0145, U+0146, U+0147, U+0148 -> 'n'
        case 0x8a: case 0x92: byte2 += 0x1; break; // U+014A -> U+014B, U+0152 -> U+0153
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91: byte1 = 'o'; byte2 = 0; break; // U+014C, U+014D, U+014E, U+014F, U+0150, U+0151 -> 'o'
        case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: byte1 = 'r'; byte2 = 0; break; // U+0154, U+0155, U+0156, U+0157, U+0158, U+0159 -> 'r'
        case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: // U+015A, U+015B, U+015C, U+015D, U+015E, U+015F
        case 0xa0: case 0xa1: case 0xbf: byte1 = 's'; byte2 = 0; break; // U+0160, U+0161, U+017F -> 's'
        case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7: byte1 = 't'; byte2 = 0; break; // U+0162, U+0163, U+0164, U+0165, U+0166, U+0167 -> 't'
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: // U+0168, U+0169, U+016A, U+016B, U+016C, U+016D
        case 0xae: case 0xaf: case 0xb0: case 0xb1: case 0xb2: case 0xb3: byte1 = 'u'; byte2 = 0; break; // U+016E, U+016F, U+0170, U+0171, U+0172, U+0173 -> 'u'
        case 0xb4: case 0xb5: byte1 = 'w'; byte2 = 0; break; // U+0174, U+0175 -> 'w'
        case 0xb6: case 0xb7: case 0xb8: byte1 = 'y'; byte2 = 0; break; // U+0176, U+0177, U+0178 -> 'y'
        case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: byte1 = 'z'; byte2 = 0; break; // U+0179, U+017A, U+017B, U+017C, U+017D, U+017E -> 'z'
      }
    break;
    case 0xc6:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: byte1 = 'b'; byte2 = 0; break; // U+0180, U+0181, U+0182, U+0183 -> 'b'
        case 0x84: case 0xa2: case 0xa7: case 0xb8: case 0xbc: byte2 += 0x1; break; // U+0184 -> U+0185, U+01A2 -> U+01A3, U+01A7 -> U+01A8, U+01B8 -> U+01B9, U+01BC -> U+01BD
        case 0x86: byte1 = 0xc9; byte2 = 0x94; break; // U+0186 -> U+0254
        case 0x87: case 0x88: byte1 = 'c'; byte2 = 0; break; // U+0187, U+0188 -> 'c'
        case 0x89: byte1 = 0xc9; byte2 = 0x96; break; // U+0189 -> U+0256
        case 0x8a: case 0x8b: case 0x8c: byte1 = 'd'; byte2 = 0; break; // U+018A, U+018B, U+018C -> 'd'
        case 0x8e: byte1 = 0xc7; byte2 = 0x9d; break; // U+018E -> U+01DD
        case 0x8f: byte1 = 0xc9; byte2 = 0x99; break; // U+018F -> U+0259
        case 0x90: byte1 = 0xc9; byte2 = 0x9b; break; // U+0190 -> U+025B
        case 0x91: case 0x92: byte1 = 'f'; byte2 = 0; break; // U+0191, U+0192 -> 'f'
        case 0x93: byte1 = 'g'; byte2 = 0; break; // U+0193 -> 'g'
        case 0x94: byte1 = 0xc9; byte2 = 0xa3; break; // U+0194 -> U+0263
        case 0x96: case 0x9c: byte1 = 0xc9; byte2 += 0x13; break; // U+0196 -> U+0269, U+019C -> U+026F
        case 0x97: byte1 = 'i'; byte2 = 0; break; // U+0197 -> 'i'
        case 0x98: case 0x99: byte1 = 'k'; byte2 = 0; break; // U+0198, U+0199 -> 'k'
        case 0x9d: case 0x9e: byte1 = 'n'; byte2 = 0; break; // U+019D, U+019E -> 'n'
        case 0x9f: case 0xa0: case 0xa1: byte1 = 'o'; byte2 = 0; break; // U+019F, U+01A0, U+01A1 -> 'o'
        case 0xa4: case 0xa5: byte1 = 'p'; byte2 = 0; break; // U+01A4, U+01A5 -> 'p'
        case 0xa6: case 0xa9: byte1 = 0xca; byte2 -= 0x26; break; // U+01A6 -> U+0280, U+01A9 -> U+0283
        case 0xac: case 0xad: case 0xae: byte1 = 't'; byte2 = 0; break; // U+01AC, U+01AD, U+01AE -> 't'
        case 0xaf: case 0xb0: byte1 = 'u'; byte2 = 0; break; // U+01AF, U+01B0 -> 'u'
        case 0xb1: byte1 = 0xca; byte2 = 0x8a; break; // U+01B1 -> U+028A
        case 0xb2: byte1 = 'v'; byte2 = 0; break; // U+01B2 -> 'v'
        case 0xb3: case 0xb4: byte1 = 'y'; byte2 = 0; break; // U+01B3, U+01B4 -> 'y'
        case 0xb5: case 0xb6: byte1 = 'z'; byte2 = 0; break; // U+01B5, U+01B6 -> 'z'
        case 0xb7: byte1 = 0xca; byte2 = 0x92; break; // U+01B7 -> U+0292
        case 0x9a: byte1 = 'l'; byte2 = 0; break; // U+019A -> 'l'
      }
    break;
    case 0xc7:
      switch (byte2)
      {
        case 0x84: case 0x87: case 0x8a: case 0xb1: byte2 += 0x2; break; // U+01C4 -> U+01C6, U+01C7 -> U+01C9, U+01CA -> U+01CC, U+01F1 -> U+01F3
        case 0x85: case 0x86: case 0xb2: case 0xb3: byte1 = 'd'; byte2 = 0; break; // U+01C5, U+01C6, U+01F2, U+01F3 -> 'd'
        case 0x88: case 0x89: byte1 = 'l'; byte2 = 0; break; // U+01C8, U+01C9 -> 'l'
        case 0x8b: case 0x8c: case 0xb8: case 0xb9: byte1 = 'n'; byte2 = 0; break; // U+01CB, U+01CC, U+01F8, U+01F9 -> 'n'
        case 0x8d: case 0x8e: case 0x9e: case 0x9f: case 0xa0: case 0xa1: // U+01CD, U+01CE, U+01DE, U+01DF, U+01E0, U+01E1
        case 0xba: case 0xbb: byte1 = 'a'; byte2 = 0; break; // U+01FA, U+01FB -> 'a'
        case 0x8f: case 0x90: byte1 = 'i'; byte2 = 0; break; // U+01CF, U+01D0 -> 'i'
        case 0x91: case 0x92: case 0xaa: case 0xab: case 0xac: case 0xad: // U+01D1, U+01D2, U+01EA, U+01EB, U+01EC, U+01ED
        case 0xbe: case 0xbf: byte1 = 'o'; byte2 = 0; break; // U+01FE, U+01FF -> 'o'
        case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: // U+01D3, U+01D4, U+01D5, U+01D6, U+01D7, U+01D8
        case 0x99: case 0x9a: case 0x9b: case 0x9c: byte1 = 'u'; byte2 = 0; break; // U+01D9, U+01DA, U+01DB, U+01DC -> 'u'
        case 0xa2: case 0xae: case 0xbc: byte2 += 0x1; break; // U+01E2 -> U+01E3, U+01EE -> U+01EF, U+01FC -> U+01FD
        case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xb4: case 0xb5: byte1 = 'g'; byte2 = 0; break; // U+01E4, U+01E5, U+01E6, U+01E7, U+01F4, U+01F5 -> 'g'
        case 0xa8: case 0xa9: byte1 = 'k'; byte2 = 0; break; // U+01E8, U+01E9 -> 'k'
        case 0xb6: byte1 = 0xc6; byte2 = 0x95; break; // U+01F6 -> U+0195
        case 0xb7: byte1 = 0xc6; byte2 = 0xbf; break; // U+01F7 -> U+01BF
      }
    break;
    case 0xc8:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0xa6: case 0xa7: byte1 = 'a'; byte2 = 0; break; // U+0200, U+0201, U+0202, U+0203, U+0226, U+0227 -> 'a'
        case 0x84: case 0x85: case 0x86: case 0x87: case 0xa8: case 0xa9: byte1 = 'e'; byte2 = 0; break; // U+0204, U+0205, U+0206, U+0207, U+0228, U+0229 -> 'e'
        case 0x88: case 0x89: case 0x8a: case 0x8b: byte1 = 'i'; byte2 = 0; break; // U+0208, U+0209, U+020A, U+020B -> 'i'
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0xaa: case 0xab: // U+020C, U+020D, U+020E, U+020F, U+022A, U+022B
        case 0xac: case 0xad: case 0xae: case 0xaf: case 0xb0: case 0xb1: byte1 = 'o'; byte2 = 0; break; // U+022C, U+022D, U+022E, U+022F, U+0230, U+0231 -> 'o'
        case 0x90: case 0x91: case 0x92: case 0x93: byte1 = 'r'; byte2 = 0; break; // U+0210, U+0211, U+0212, U+0213 -> 'r'
        case 0x94: case 0x95: case 0x96: case 0x97: byte1 = 'u'; byte2 = 0; break; // U+0214, U+0215, U+0216, U+0217 -> 'u'
        case 0x98: case 0x99: byte1 = 's'; byte2 = 0; break; // U+0218, U+0219 -> 's'
        case 0x9a: case 0x9b: byte1 = 't'; byte2 = 0; break; // U+021A, U+021B -> 't'
        case 0x9c: case 0xa2: byte2 += 0x1; break; // U+021C -> U+021D, U+0222 -> U+0223
        case 0x9e: case 0x9f: byte1 = 'h'; byte2 = 0; break; // U+021E, U+021F -> 'h'
        case 0xa0: byte1 = 'n'; byte2 = 0; break; // U+0220 -> 'n'
        case 0xa4: case 0xa5: byte1 = 'z'; byte2 = 0; break; // U+0224, U+0225 -> 'z'
        case 0xb2: case 0xb3: byte1 = 'y'; byte2 = 0; break; // U+0232, U+0233 -> 'y'
        case 0xbb: case 0xbc: byte1 = 'c'; byte2 = 0; break; // U+023B, U+023C -> 'c'
        case 0xbd: byte1 = 'l'; byte2 = 0; break; // U+023D -> 'l'
      }
    break;
    case 0xc9:
      switch (byte2)
      {
        case 0x83: case 0x93: byte1 = 'b'; byte2 = 0; break; // U+0243, U+0253 -> 'b'
        case 0x97: byte1 = 'd'; byte2 = 0; break; // U+0257 -> 'd'
        case 0xa0: byte1 = 'g'; byte2 = 0; break; // U+0260 -> 'g'
        case 0xa8: byte1 = 'i'; byte2 = 0; break; // U+0268 -> 'i'
        case 0xb2: byte1 = 'n'; byte2 = 0; break; // U+0272 -> 'n'
        case 0xb5: byte1 = 'o'; byte2 = 0; break; // U+0275 -> 'o'
        case 0x81: case 0x8a: byte2 += 0x1; break; // U+0241 -> U+0242, U+024A -> U+024B
        case 0x84: byte1 = 'u'; byte2 = 0; break; // U+0244 -> 'u'
        case 0x85: byte1 = 0xca; byte2 = 0x8c; break; // U+0245 -> U+028C
        case 0x86: case 0x87: byte1 = 'e'; byte2 = 0; break; // U+0246, U+0247 -> 'e'
        case 0x88: case 0x89: byte1 = 'j'; byte2 = 0; break; // U+0248, U+0249 -> 'j'
        case 0x8c: case 0x8d: byte1 = 'r'; byte2 = 0; break; // U+024C, U+024D -> 'r'
        case 0x8e: case 0x8f: byte1 = 'y'; byte2 = 0; break; // U+024E, U+024F -> 'y'
      }
    break;
    case 0xca:
      switch (byte2)
      {
        case 0x88: byte1 = 't'; byte2 = 0; break; // U+0288 -> 't'
        case 0x8b: byte1 = 'v'; byte2 = 0; break; // U+028B -> 'v'
        case 0x89: byte1 = 'u'; byte2 = 0; break; // U+0289 -> 'u'
      }
    break;
    case 0xcd:
      switch (byte2)
      {
        case 0x85: byte1 = 0xce; byte2 = 0xb9; break; // U+0345 -> U+03B9
        case 0xb0: case 0xb2: case 0xb6: byte2 += 0x1; break; // U+0370 -> U+0371, U+0372 -> U+0373, U+0376 -> U+0377
        case 0xbf: byte1 = 0xcf; byte2 = 0xb3; break; // U+037F -> U+03F3
      }
    break;
    case 0xce:
      switch (byte2)
      {
        case 0x86: byte2 = 0xac; break; // U+0386 -> U+03AC
        case 0x88: case 0x89: case 0x8a: byte2 += 0x25; break; // U+0388 -> U+03AD, U+0389 -> U+03AE, U+038A -> U+03AF
        case 0x8c: byte1 = 0xcf; break; // U+038C
        case 0x8e: case 0x8f: byte1 = 0xcf; byte2 -= 0x1; break; // U+038E -> U+03CD, U+038F -> U+03CE
        case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: // U+0391 -> U+03B1, U+0392 -> U+03B2, U+0393 -> U+03B3, U+0394 -> U+03B4, U+0395 -> U+03B5, U+0396 -> U+03B6
        case 0x97: case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: // U+0397 -> U+03B7, U+0398 -> U+03B8, U+0399 -> U+03B9, U+039A -> U+03BA, U+039B -> U+03BB, U+039C -> U+03BC, U+039D -> U+03BD
        case 0x9e: case 0x9f: byte2 += 0x20; break; // U+039E -> U+03BE, U+039F -> U+03BF
        case 0xa0: case 0xa1: case 0xa3: case 0xa4: case 0xa5: case 0xa6: // U+03A0 -> U+03C0, U+03A1 -> U+03C1, U+03A3 -> U+03C3, U+03A4 -> U+03C4, U+03A5 -> U+03C5, U+03A6 -> U+03C6
        case 0xa7: case 0xa8: case 0xa9: case 0xaa: case 0xab: byte1 = 0xcf; byte2 -= 0x20; break; // U+03A7 -> U+03C7, U+03A8 -> U+03C8, U+03A9 -> U+03C9, U+03AA -> U+03CA, U+03AB -> U+03CB
      }
    break;
    case 0xcf:
      switch (byte2)
      {
        case 0x82: case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0: // U+03C2 -> U+03C3, U+03D8 -> U+03D9, U+03DA -> U+03DB, U+03DC -> U+03DD, U+03DE -> U+03DF, U+03E0 -> U+03E1
        case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae: // U+03E2 -> U+03E3, U+03E4 -> U+03E5, U+03E6 -> U+03E7, U+03E8 -> U+03E9, U+03EA -> U+03EB, U+03EC -> U+03ED, U+03EE -> U+03EF
        case 0xb7: case 0xba: byte2 += 0x1; break; // U+03F7 -> U+03F8, U+03FA -> U+03FB
        case 0x8f: byte2 = 0x97; break; // U+03CF -> U+03D7
        case 0x90: byte1 = 0xce; byte2 = 0xb2; break; // U+03D0 -> U+03B2
        case 0x91: byte1 = 0xce; byte2 = 0xb8; break; // U+03D1 -> U+03B8
        case 0x95: byte2 = 0x86; break; // U+03D5 -> U+03C6
        case 0x96: byte2 = 0x80; break; // U+03D6 -> U+03C0
        case 0xb0: byte1 = 0xce; byte2 = 0xba; break; // U+03F0 -> U+03BA
        case 0xb1: byte2 = 0x81; break; // U+03F1 -> U+03C1
        case 0xb4: byte1 = 0xce; byte2 = 0xb8; break; // U+03F4 -> U+03B8
        case 0xb5: byte1 = 0xce; break; // U+03F5
        case 0xb9: byte2 = 0xb2; break; // U+03F9 -> U+03F2
        case 0xbd: case 0xbe: case 0xbf: byte1 = 0xcd; byte2 -= 0x2; break; // U+03FD -> U+037B, U+03FE -> U+037C, U+03FF -> U+037D
      }
    break;
    case 0xd0:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: // U+0400 -> U+0450, U+0401 -> U+0451, U+0402 -> U+0452, U+0403 -> U+0453, U+0404 -> U+0454, U+0405 -> U+0455
        case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: // U+0406 -> U+0456, U+0407 -> U+0457, U+0408 -> U+0458, U+0409 -> U+0459, U+040A -> U+045A, U+040B -> U+045B, U+040C -> U+045C
        case 0x8d: case 0x8e: case 0x8f: byte1 = 0xd1; byte2 += 0x10; break; // U+040D -> U+045D, U+040E -> U+045E, U+040F -> U+045F
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: // U+0410 -> U+0430, U+0411 -> U+0431, U+0412 -> U+0432, U+0413 -> U+0433, U+0414 -> U+0434, U+0415 -> U+0435
        case 0x96: case 0x97: case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: // U+0416 -> U+0436, U+0417 -> U+0437, U+0418 -> U+0438, U+0419 -> U+0439, U+041A -> U+043A, U+041B -> U+043B, U+041C -> U+043C
        case 0x9d: case 0x9e: case 0x9f: byte2 += 0x20; break; // U+041D -> U+043D, U+041E -> U+043E, U+041F -> U+043F
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: // U+0420 -> U+0440, U+0421 -> U+0441, U+0422 -> U+0442, U+0423 -> U+0443, U+0424 -> U+0444, U+0425 -> U+0445
        case 0xa6: case 0xa7: case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: // U+0426 -> U+0446, U+0427 -> U+0447, U+0428 -> U+0448, U+0429 -> U+0449, U+042A -> U+044A, U+042B -> U+044B, U+042C -> U+044C
        case 0xad: case 0xae: case 0xaf: byte1 = 0xd1; byte2 -= 0x20; break; // U+042D -> U+044D, U+042E -> U+044E, U+042F -> U+044F
      }
    break;
    case 0xd1:
      switch (byte2)
      {
        case 0xa0: case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa: // U+0460 -> U+0461, U+0462 -> U+0463, U+0464 -> U+0465, U+0466 -> U+0467, U+0468 -> U+0469, U+046A -> U+046B
        case 0xac: case 0xae: case 0xb0: case 0xb2: case 0xb4: case 0xb6: case 0xb8: // U+046C -> U+046D, U+046E -> U+046F, U+0470 -> U+0471, U+0472 -> U+0473, U+0474 -> U+0475, U+0476 -> U+0477, U+0478 -> U+0479
        case 0xba: case 0xbc: case 0xbe: byte2 += 0x1; break; // U+047A -> U+047B, U+047C -> U+047D, U+047E -> U+047F
      }
    break;
    case 0xd2:
      switch (byte2)
      {
        case 0x80: case 0x8a: case 0x8c: case 0x8e: case 0x90: case 0x92: // U+0480 -> U+0481, U+048A -> U+048B, U+048C -> U+048D, U+048E -> U+048F, U+0490 -> U+0491, U+0492 -> U+0493
        case 0x94: case 0x96: case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0: // U+0494 -> U+0495, U+0496 -> U+0497, U+0498 -> U+0499, U+049A -> U+049B, U+049C -> U+049D, U+049E -> U+049F, U+04A0 -> U+04A1
        case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae: // U+04A2 -> U+04A3, U+04A4 -> U+04A5, U+04A6 -> U+04A7, U+04A8 -> U+04A9, U+04AA -> U+04AB, U+04AC -> U+04AD, U+04AE -> U+04AF
        case 0xb0: case 0xb2: case 0xb4: case 0xb6: case 0xb8: case 0xba: case 0xbc: // U+04B0 -> U+04B1, U+04B2 -> U+04B3, U+04B4 -> U+04B5, U+04B6 -> U+04B7, U+04B8 -> U+04B9, U+04BA -> U+04BB, U+04BC -> U+04BD
        case 0xbe: byte2 += 0x1; break; // U+04BE -> U+04BF
      }
    break;
    case 0xd3:
      switch (byte2)
      {
        case 0x80: byte2 = 0x8f; break; // U+04C0 -> U+04CF
        case 0x81: case 0x83: case 0x85: case 0x87: case 0x89: case 0x8b: // U+04C1 -> U+04C2, U+04C3 -> U+04C4, U+04C5 -> U+04C6, U+04C7 -> U+04C8, U+04C9 -> U+04CA, U+04CB -> U+04CC
        case 0x8d: case 0x90: case 0x92: case 0x94: case 0x96: case 0x98: case 0x9a: // U+04CD -> U+04CE, U+04D0 -> U+04D1, U+04D2 -> U+04D3, U+04D4 -> U+04D5, U+04D6 -> U+04D7, U+04D8 -> U+04D9, U+04DA -> U+04DB
        case 0x9c: case 0x9e: case 0xa0: case 0xa2: case 0xa4: case 0xa6: case 0xa8: // U+04DC -> U+04DD, U+04DE -> U+04DF, U+04E0 -> U+04E1, U+04E2 -> U+04E3, U+04E4 -> U+04E5, U+04E6 -> U+04E7, U+04E8 -> U+04E9
        case 0xaa: case 0xac: case 0xae: case 0xb0: case 0xb2: case 0xb4: case 0xb6: // U+04EA -> U+04EB, U+04EC -> U+04ED, U+04EE -> U+04EF, U+04F0 -> U+04F1, U+04F2 -> U+04F3, U+04F4 -> U+04F5, U+04F6 -> U+04F7
        case 0xb8: case 0xba: case 0xbc: case 0xbe: byte2 += 0x1; break; // U+04F8 -> U+04F9, U+04FA -> U+04FB, U+04FC -> U+04FD, U+04FE -> U+04FF
      }
    break;
    case 0xd4:
      switch (byte2)
      {
        case 0x80: case 0x82: case 0x84: case 0x86: case 0x88: case 0x8a: // U+0500 -> U+0501, U+0502 -> U+0503, U+0504 -> U+0505, U+0506 -> U+0507, U+0508 -> U+0509, U+050A -> U+050B
        case 0x8c: case 0x8e: case 0x90: case 0x92: case 0x94: case 0x96: case 0x98: // U+050C -> U+050D, U+050E -> U+050F, U+0510 -> U+0511, U+0512 -> U+0513, U+0514 -> U+0515, U+0516 -> U+0517, U+0518 -> U+0519
        case 0x9a: case 0x9c: case 0x9e: case 0xa0: case 0xa2: case 0xa4: case 0xa6: // U+051A -> U+051B, U+051C -> U+051D, U+051E -> U+051F, U+0520 -> U+0521, U+0522 -> U+0523, U+0524 -> U+0525, U+0526 -> U+0527
        case 0xa8: case 0xaa: case 0xac: case 0xae: byte2 += 0x1; break; // U+0528 -> U+0529, U+052A -> U+052B, U+052C -> U+052D, U+052E -> U+052F
        case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: // U+0531 -> U+0561, U+0532 -> U+0562, U+0533 -> U+0563, U+0534 -> U+0564, U+0535 -> U+0565, U+0536 -> U+0566
        case 0xb7: case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: // U+0537 -> U+0567, U+0538 -> U+0568, U+0539 -> U+0569, U+053A -> U+056A, U+053B -> U+056B, U+053C -> U+056C, U+053D -> U+056D
        case 0xbe: case 0xbf: byte1 = 0xd5; byte2 -= 0x10; break; // U+053E -> U+056E, U+053F -> U+056F
      }
    break;
    case 0xd5:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: // U+0540 -> U+0570, U+0541 -> U+0571, U+0542 -> U+0572, U+0543 -> U+0573, U+0544 -> U+0574, U+0545 -> U+0575
        case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: // U+0546 -> U+0576, U+0547 -> U+0577, U+0548 -> U+0578, U+0549 -> U+0579, U+054A -> U+057A, U+054B -> U+057B, U+054C -> U+057C
        case 0x8d: case 0x8e: case 0x8f: byte2 += 0x30; break; // U+054D -> U+057D, U+054E -> U+057E, U+054F -> U+057F
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: // U+0550 -> U+0580, U+0551 -> U+0581, U+0552 -> U+0582, U+0553 -> U+0583, U+0554 -> U+0584, U+0555 -> U+0585
        case 0x96: byte1 = 0xd6; byte2 -= 0x10; break; // U+0556 -> U+0586
      }
    break;
  }
}

static WDL_STATICFUNC_UNUSED char wdl_utf8_2byte_casefold1(unsigned char byte1, unsigned char byte2)
{
  wdl_utf8_2byte_casefold(byte1,byte2);
  return byte1;
}

static WDL_STATICFUNC_UNUSED const unsigned char *wdl_utf8_scan_unfolded(const unsigned char *src, unsigned char c) {
  #define SCAN(exp) for(;;) { const unsigned char v = *src; if (!v) break; if (exp) return src; src++; }
  switch (c)
  {
    case 'a': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc4 || v == 0xc7 || v == 0xc8)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'b': SCAN((v|0x20) == c || ((v == 0xc6 || v == 0xc9) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'c': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc4 || v == 0xc6 || v == 0xc8)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'd': SCAN((v|0x20) == c || ((v >= 0xc4 && (v == 0xc4 || v == 0xc6 || v == 0xc7 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'e': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc4 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'f': SCAN((v|0x20) == c || ((v == 0xc6) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'g': SCAN((v|0x20) == c || ((v >= 0xc4 && (v == 0xc4 || v == 0xc6 || v == 0xc7 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'h': SCAN((v|0x20) == c || ((v == 0xc4 || v == 0xc8) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'i': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc4 || v == 0xc6 || v == 0xc7 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'j': SCAN((v|0x20) == c || ((v == 0xc4 || v == 0xc9) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'k': SCAN((v|0x20) == c || ((v >= 0xc4 && (v == 0xc4 || v == 0xc6 || v == 0xc7)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'l': SCAN((v|0x20) == c || ((v >= 0xc4 && (v == 0xc4 || v == 0xc5 || v == 0xc6 || v == 0xc7 || v == 0xc8)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'n': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc5 || v == 0xc6 || v == 0xc7 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'o': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc5 || v == 0xc6 || v == 0xc7 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'p': SCAN((v|0x20) == c || ((v == 0xc6) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'r': SCAN((v|0x20) == c || ((v >= 0xc5 && (v == 0xc5 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 's': SCAN((v|0x20) == c || ((v == 0xc5 || v == 0xc8) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 't': SCAN((v|0x20) == c || ((v >= 0xc5 && (v == 0xc5 || v == 0xc6 || v == 0xc8 || v == 0xca)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'u': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc5 || v == 0xc6 || v == 0xc7 || v == 0xc8 || v == 0xc9 || v == 0xca)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'v': SCAN((v|0x20) == c || ((v == 0xc6 || v == 0xca) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'w': SCAN((v|0x20) == c || ((v == 0xc5) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'y': SCAN((v|0x20) == c || ((v >= 0xc3 && (v == 0xc3 || v == 0xc5 || v == 0xc6 || v == 0xc8 || v == 0xc9)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 'z': SCAN((v|0x20) == c || ((v >= 0xc5 && (v == 0xc5 || v == 0xc6 || v == 0xc8)) && wdl_utf8_2byte_casefold1(v,src[1])==c)) break;
    case 0xc6: SCAN(v == c || (v == 0xc7)) break;
    case 0xc7: SCAN(v == c || (v == 0xc6)) break;
    case 0xc9: SCAN(v == c || (v == 0xc6)) break;
    case 0xca: SCAN(v == c || (v == 0xc6 || v == 0xc9)) break;
    case 0xcd: SCAN(v == c || (v == 0xcf)) break;
    case 0xce: SCAN(v == c || (v >= 0xc2 && (v == 0xc2 || v == 0xcd || v == 0xcf))) break;
    case 0xcf: SCAN(v == c || (v == 0xcd || v == 0xce)) break;
    case 0xd1: SCAN(v == c || (v == 0xd0)) break;
    case 0xd5: SCAN(v == c || (v == 0xd4)) break;
    case 0xd6: SCAN(v == c || (v == 0xd5)) break;
    default: if (c>='a'&&c<='z') SCAN((v|0x20) == c) else SCAN(v == c); break;
  }
  #undef SCAN
  return NULL;
}

#endif
