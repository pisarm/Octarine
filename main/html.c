/*-
 * Copyright (c) 2017 Flemming Pedersen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
const char *html_template = "<!DOCTYPE html>"
                            "<html>"
                            "<head>"
                            "<style>"
                            "ul {"
                            "list-style-type: none;"
                            "margin: 0;"
                            "padding: 0;"
                            "overflow: hidden;"
                            "background-color: #3D096B;"
                            "}"
                            "li {"
                            "float: left;"
                            "}"
                            "li a, .dropbtn {"
                            "display: inline-block;"
                            "color: white;"
                            "text-align: center;"
                            "padding: 14px 16px;"
                            "text-decoration: none;"
                            "}"
                            "li a:hover, .dropdown:hover .dropbtn {"
                            "background-color: #2e0453;"
                            "}"
                            "li.dropdown {"
                            "display: inline-block;"
                            "}"
                            ".dropdown-content {"
                            "display: none;"
                            "position: absolute;"
                            "background-color: #f9f9f9;"
                            "min-width: 160px;"
                            "box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2);"
                            "z-index: 1;"
                            "}"
                            ".dropdown-content a {"
                            "color: black;"
                            "padding: 12px 16px;"
                            "text-decoration: none;"
                            "display: block;"
                            "text-align: left;"
                            "}"
                            ".dropdown-content a:hover {"
                            "background-color: #2e0453;"
                            "color: white;"
                            "}"
                            ".dropdown:hover .dropdown-content {"
                            "display: block;"
                            "}"
                            "</style>"
                            "</head>"
                            "<body>"
                            "<ul>"
                            "<li><a href='/'>OCTARINE</a></li>"
                            "<li class='dropdown'>"
                            "<a href='javascript:void(0)' class='dropbtn'>CONFIG</a>"
                            "<div class='dropdown-content'>"
                            "<a href='/config/time'>TIME</a>"
                            "<a href='/config/wifi'>WIFI</a>"
                            "</div>"
                            "</li>"
                            "<li><a href='/about'>ABOUT</a></li>"
                            "</ul>"
                            "%s"
                            "</body>"
                            "</html>";

const char *html_content_root = "<p>&#x1F6A7;</p>";

const char *html_content_config_time = "<p><h3>Input TZ time zone variable:</h3></p>"
                                       "<form action='/config/time' method='post'>"
                                       "<input type='text' name='timezone' value=''>"
                                       "<br>"
                                       "<input type='submit' value='Set'>"
                                       "</form>"
                                       "<br>"
                                       "<p>Time zone is set by using a POSIX TZ environment variable.</p>"
                                       "<p>See <a href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>this link</a> for more help.</p>"
                                       "<p>The system will fall back to UTC if no time zone is set.</p>"
                                       "<br>"
                                       "<p><h3>Examples</h3></p>"
                                       "<p>CET-1CEST,M3.5.0/2,M10.5.0/3 is Europe/Copenhagen (including intervals for Daylight Saving Time)</p>";

const char *html_content_config_wifi = "<p>Enter the requisite WIFI credentials below.</p>"
                                       "<form action='/config/wifi' method='post'>SSID<br>"
                                       "<input type='text' name='ssid' value=''><br>Password<br>"
                                       "<input type='text' name='password' value=''><br>"
                                       "<input type='submit' value='Set'>"
                                       "</form>";

const char *html_content_about = "<p>ESP-IDF %s</p>"
                                 "<p>Free heap: %u bytes</p>";
