/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package org.apache.felix.sigil.common.osgi;


public class LDAPParseException extends RuntimeException
{
    private static final long serialVersionUID = 2L;

    private ParseState ps;
    private static final String LINE_SEPARATOR = System.getProperty( "line.separator", "\\r\\n" );


    public LDAPParseException( String message, ParseState ps )
    {
        super( message );
        this.ps = ps;
    }


    public LDAPParseException( String message )
    {
        super( message );
    }


    @Override
    public String getMessage()
    {
        if ( ps == null )
        {
            return super.getMessage();
        }

        String basicMessage = super.getMessage();
        StringBuffer buf = new StringBuffer( basicMessage.length() + ps.str.length() * 2 );
        buf.append( basicMessage ).append( LINE_SEPARATOR );
        buf.append( ps.str ).append( LINE_SEPARATOR );
        for ( int i = 0; i < ps.pos; i++ )
        {
            buf.append( " " );
        }
        buf.append( "^" );
        return buf.toString();
    }

}
