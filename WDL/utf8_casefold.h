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

while ($x = fgets($fp))
{
  $a = sscanf($x, "%x; %c; %x");
  if (!isset($a[2])) continue;
  if ($a[1] == 'F' || $a[0] < 0x80 || $a[0] >= 0x800 || $a[2] >= 0x800) continue;
  $b2 = 0x80 + ($a[0]&0x3f);
  $b1 = 0xc0 + ($a[0]>>6);

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
    foreach ($list as $b2) {
      if ($cc++ >= 8) { printf("\n       "); $cc=0; }
      printf(" case 0x%x:",$b2);
    }
    $adj = sscanf($offs, "%d %d");
    if ($adj[0] != $b1) printf($adj[0] < 0x80 ? " byte1 = '%c';" : " byte1 = 0x%x;",$adj[0]);

    if (!isset($adj[1])) printf(" byte2 = 0;");
    else if ($adj[1] != 0 && count($list)==1) printf(" byte2 = 0x%x;",$list[0] + $adj[1]);
    else if ($adj[1] > 0) printf(" byte2 += 0x%x;",$adj[1]);
    else if ($adj[1] < 0) printf(" byte2 -= 0x%x;",-$adj[1]);
    printf(" break;\n");
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
        case 0xb5: byte1 = 0xce; byte2 = 0xbc; break;
      }
    break;
    case 0xc3:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0xa0: case 0xa1:
        case 0xa2: case 0xa3: case 0xa4: case 0xa5: byte1 = 'a'; byte2 = 0; break;
        case 0x86: case 0x90: case 0x9e: byte2 += 0x20; break;
        case 0x87: case 0xa7: byte1 = 'c'; byte2 = 0; break;
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0xa8: case 0xa9: case 0xaa: case 0xab: byte1 = 'e'; byte2 = 0; break;
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0xac: case 0xad: case 0xae: case 0xaf: byte1 = 'i'; byte2 = 0; break;
        case 0x91: case 0xb1: byte1 = 'n'; byte2 = 0; break;
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x98: case 0xb2: case 0xb3:
        case 0xb4: case 0xb5: case 0xb6: case 0xb8: byte1 = 'o'; byte2 = 0; break;
        case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0xb9: case 0xba: case 0xbb: case 0xbc: byte1 = 'u'; byte2 = 0; break;
        case 0x9d: case 0xbd: case 0xbf: byte1 = 'y'; byte2 = 0; break;
      }
    break;
    case 0xc4:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: byte1 = 'a'; byte2 = 0; break;
        case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: byte1 = 'c'; byte2 = 0; break;
        case 0x8e: case 0x8f: case 0x90: case 0x91: byte1 = 'd'; byte2 = 0; break;
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99:
        case 0x9a: case 0x9b: byte1 = 'e'; byte2 = 0; break;
        case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xa0: case 0xa1: case 0xa2: case 0xa3: byte1 = 'g'; byte2 = 0; break;
        case 0xa4: case 0xa5: case 0xa6: case 0xa7: byte1 = 'h'; byte2 = 0; break;
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: byte1 = 'i'; byte2 = 0; break;
        case 0xb2: byte2 = 0xb3; break;
        case 0xb4: case 0xb5: byte1 = 'j'; byte2 = 0; break;
        case 0xb6: case 0xb7: byte1 = 'k'; byte2 = 0; break;
        case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: byte1 = 'l'; byte2 = 0; break;
      }
    break;
    case 0xc5:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: byte1 = 'l'; byte2 = 0; break;
        case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: byte1 = 'n'; byte2 = 0; break;
        case 0x8a: case 0x92: byte2 += 0x1; break;
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91: byte1 = 'o'; byte2 = 0; break;
        case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: byte1 = 'r'; byte2 = 0; break;
        case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xa0: case 0xa1:
        case 0xbf: byte1 = 's'; byte2 = 0; break;
        case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7: byte1 = 't'; byte2 = 0; break;
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: byte1 = 'u'; byte2 = 0; break;
        case 0xb4: case 0xb5: byte1 = 'w'; byte2 = 0; break;
        case 0xb6: case 0xb7: case 0xb8: byte1 = 'y'; byte2 = 0; break;
        case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: byte1 = 'z'; byte2 = 0; break;
      }
    break;
    case 0xc6:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: byte1 = 'b'; byte2 = 0; break;
        case 0x84: case 0xa2: case 0xa7: case 0xb8: case 0xbc: byte2 += 0x1; break;
        case 0x86: byte1 = 0xc9; byte2 = 0x94; break;
        case 0x87: case 0x88: byte1 = 'c'; byte2 = 0; break;
        case 0x89: byte1 = 0xc9; byte2 = 0x96; break;
        case 0x8a: case 0x8b: case 0x8c: byte1 = 'd'; byte2 = 0; break;
        case 0x8e: byte1 = 0xc7; byte2 = 0x9d; break;
        case 0x8f: byte1 = 0xc9; byte2 = 0x99; break;
        case 0x90: byte1 = 0xc9; byte2 = 0x9b; break;
        case 0x91: case 0x92: byte1 = 'f'; byte2 = 0; break;
        case 0x93: byte1 = 'g'; byte2 = 0; break;
        case 0x94: byte1 = 0xc9; byte2 = 0xa3; break;
        case 0x96: case 0x9c: byte1 = 0xc9; byte2 += 0x13; break;
        case 0x97: byte1 = 'i'; byte2 = 0; break;
        case 0x98: case 0x99: byte1 = 'k'; byte2 = 0; break;
        case 0x9d: case 0x9e: byte1 = 'n'; byte2 = 0; break;
        case 0x9f: case 0xa0: case 0xa1: byte1 = 'o'; byte2 = 0; break;
        case 0xa4: case 0xa5: byte1 = 'p'; byte2 = 0; break;
        case 0xa6: case 0xa9: byte1 = 0xca; byte2 -= 0x26; break;
        case 0xac: case 0xad: case 0xae: byte1 = 't'; byte2 = 0; break;
        case 0xaf: case 0xb0: byte1 = 'u'; byte2 = 0; break;
        case 0xb1: byte1 = 0xca; byte2 = 0x8a; break;
        case 0xb2: byte1 = 'v'; byte2 = 0; break;
        case 0xb3: case 0xb4: byte1 = 'y'; byte2 = 0; break;
        case 0xb5: case 0xb6: byte1 = 'z'; byte2 = 0; break;
        case 0xb7: byte1 = 0xca; byte2 = 0x92; break;
        case 0x9a: byte1 = 'l'; byte2 = 0; break;
      }
    break;
    case 0xc7:
      switch (byte2)
      {
        case 0x84: case 0x87: case 0x8a: case 0xb1: byte2 += 0x2; break;
        case 0x85: case 0x86: case 0xb2: case 0xb3: byte1 = 'd'; byte2 = 0; break;
        case 0x88: case 0x89: byte1 = 'l'; byte2 = 0; break;
        case 0x8b: case 0x8c: case 0xb8: case 0xb9: byte1 = 'n'; byte2 = 0; break;
        case 0x8d: case 0x8e: case 0x9e: case 0x9f: case 0xa0: case 0xa1: case 0xba: case 0xbb: byte1 = 'a'; byte2 = 0; break;
        case 0x8f: case 0x90: byte1 = 'i'; byte2 = 0; break;
        case 0x91: case 0x92: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xbe: case 0xbf: byte1 = 'o'; byte2 = 0; break;
        case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: case 0x9a:
        case 0x9b: case 0x9c: byte1 = 'u'; byte2 = 0; break;
        case 0xa2: case 0xae: case 0xbc: byte2 += 0x1; break;
        case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xb4: case 0xb5: byte1 = 'g'; byte2 = 0; break;
        case 0xa8: case 0xa9: byte1 = 'k'; byte2 = 0; break;
        case 0xb6: byte1 = 0xc6; byte2 = 0x95; break;
        case 0xb7: byte1 = 0xc6; byte2 = 0xbf; break;
      }
    break;
    case 0xc8:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0xa6: case 0xa7: byte1 = 'a'; byte2 = 0; break;
        case 0x84: case 0x85: case 0x86: case 0x87: case 0xa8: case 0xa9: byte1 = 'e'; byte2 = 0; break;
        case 0x88: case 0x89: case 0x8a: case 0x8b: byte1 = 'i'; byte2 = 0; break;
        case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0xaa: case 0xab: case 0xac: case 0xad:
        case 0xae: case 0xaf: case 0xb0: case 0xb1: byte1 = 'o'; byte2 = 0; break;
        case 0x90: case 0x91: case 0x92: case 0x93: byte1 = 'r'; byte2 = 0; break;
        case 0x94: case 0x95: case 0x96: case 0x97: byte1 = 'u'; byte2 = 0; break;
        case 0x98: case 0x99: byte1 = 's'; byte2 = 0; break;
        case 0x9a: case 0x9b: byte1 = 't'; byte2 = 0; break;
        case 0x9c: case 0xa2: byte2 += 0x1; break;
        case 0x9e: case 0x9f: byte1 = 'h'; byte2 = 0; break;
        case 0xa0: byte1 = 'n'; byte2 = 0; break;
        case 0xa4: case 0xa5: byte1 = 'z'; byte2 = 0; break;
        case 0xb2: case 0xb3: byte1 = 'y'; byte2 = 0; break;
        case 0xbb: case 0xbc: byte1 = 'c'; byte2 = 0; break;
        case 0xbd: byte1 = 'l'; byte2 = 0; break;
      }
    break;
    case 0xc9:
      switch (byte2)
      {
        case 0x83: case 0x93: byte1 = 'b'; byte2 = 0; break;
        case 0x97: byte1 = 'd'; byte2 = 0; break;
        case 0xa0: byte1 = 'g'; byte2 = 0; break;
        case 0xa8: byte1 = 'i'; byte2 = 0; break;
        case 0xb2: byte1 = 'n'; byte2 = 0; break;
        case 0xb5: byte1 = 'o'; byte2 = 0; break;
        case 0x81: case 0x8a: byte2 += 0x1; break;
        case 0x84: byte1 = 'u'; byte2 = 0; break;
        case 0x85: byte1 = 0xca; byte2 = 0x8c; break;
        case 0x86: case 0x87: byte1 = 'e'; byte2 = 0; break;
        case 0x88: case 0x89: byte1 = 'j'; byte2 = 0; break;
        case 0x8c: case 0x8d: byte1 = 'r'; byte2 = 0; break;
        case 0x8e: case 0x8f: byte1 = 'y'; byte2 = 0; break;
      }
    break;
    case 0xca:
      switch (byte2)
      {
        case 0x88: byte1 = 't'; byte2 = 0; break;
        case 0x8b: byte1 = 'v'; byte2 = 0; break;
        case 0x89: byte1 = 'u'; byte2 = 0; break;
      }
    break;
    case 0xcd:
      switch (byte2)
      {
        case 0x85: byte1 = 0xce; byte2 = 0xb9; break;
        case 0xb0: case 0xb2: case 0xb6: byte2 += 0x1; break;
        case 0xbf: byte1 = 0xcf; byte2 = 0xb3; break;
      }
    break;
    case 0xce:
      switch (byte2)
      {
        case 0x86: byte2 = 0xac; break;
        case 0x88: case 0x89: case 0x8a: byte2 += 0x25; break;
        case 0x8c: byte1 = 0xcf; break;
        case 0x8e: case 0x8f: byte1 = 0xcf; byte2 -= 0x1; break;
        case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
        case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: byte2 += 0x20; break;
        case 0xa0: case 0xa1: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xa8:
        case 0xa9: case 0xaa: case 0xab: byte1 = 0xcf; byte2 -= 0x20; break;
      }
    break;
    case 0xcf:
      switch (byte2)
      {
        case 0x82: case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0: case 0xa2: case 0xa4:
        case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae: case 0xb7: case 0xba: byte2 += 0x1; break;
        case 0x8f: byte2 = 0x97; break;
        case 0x90: byte1 = 0xce; byte2 = 0xb2; break;
        case 0x91: byte1 = 0xce; byte2 = 0xb8; break;
        case 0x95: byte2 = 0x86; break;
        case 0x96: byte2 = 0x80; break;
        case 0xb0: byte1 = 0xce; byte2 = 0xba; break;
        case 0xb1: byte2 = 0x81; break;
        case 0xb4: byte1 = 0xce; byte2 = 0xb8; break;
        case 0xb5: byte1 = 0xce; break;
        case 0xb9: byte2 = 0xb2; break;
        case 0xbd: case 0xbe: case 0xbf: byte1 = 0xcd; byte2 -= 0x2; break;
      }
    break;
    case 0xd0:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f: byte1 = 0xd1; byte2 += 0x10; break;
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f: byte2 += 0x20; break;
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf: byte1 = 0xd1; byte2 -= 0x20; break;
      }
    break;
    case 0xd1:
      switch (byte2)
      {
        case 0xa0: case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae:
        case 0xb0: case 0xb2: case 0xb4: case 0xb6: case 0xb8: case 0xba: case 0xbc: case 0xbe: byte2 += 0x1; break;
      }
    break;
    case 0xd2:
      switch (byte2)
      {
        case 0x80: case 0x8a: case 0x8c: case 0x8e: case 0x90: case 0x92: case 0x94: case 0x96:
        case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0: case 0xa2: case 0xa4: case 0xa6: case 0xa8:
        case 0xaa: case 0xac: case 0xae: case 0xb0: case 0xb2: case 0xb4: case 0xb6: case 0xb8: case 0xba:
        case 0xbc: case 0xbe: byte2 += 0x1; break;
      }
    break;
    case 0xd3:
      switch (byte2)
      {
        case 0x80: byte2 = 0x8f; break;
        case 0x81: case 0x83: case 0x85: case 0x87: case 0x89: case 0x8b: case 0x8d: case 0x90:
        case 0x92: case 0x94: case 0x96: case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0: case 0xa2:
        case 0xa4: case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae: case 0xb0: case 0xb2: case 0xb4:
        case 0xb6: case 0xb8: case 0xba: case 0xbc: case 0xbe: byte2 += 0x1; break;
      }
    break;
    case 0xd4:
      switch (byte2)
      {
        case 0x80: case 0x82: case 0x84: case 0x86: case 0x88: case 0x8a: case 0x8c: case 0x8e:
        case 0x90: case 0x92: case 0x94: case 0x96: case 0x98: case 0x9a: case 0x9c: case 0x9e: case 0xa0:
        case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa: case 0xac: case 0xae: byte2 += 0x1; break;
        case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7: case 0xb8:
        case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: byte1 = 0xd5; byte2 -= 0x10; break;
      }
    break;
    case 0xd5:
      switch (byte2)
      {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f: byte2 += 0x30; break;
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: byte1 = 0xd6; byte2 -= 0x10; break;
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
