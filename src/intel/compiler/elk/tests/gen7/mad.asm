mad(8)          g11<1>F         g4.7<0,1,0>F    g4.3<0,1,0>F    g9<4,4,1>F { align16 1Q };
mad(8)          g25<1>F         g6.7<0,1,0>F    g6.3<0,1,0>F    g21<4,4,1>F { align16 2Q };
mad(8)          g18<1>.xyzF     -g16<4,4,1>.xyzzF g11<4,4,1>.xyzzF g9<4,4,1>.xyzzF { align16 1Q };
mad(8)          g3<1>F          -g2.4<0,1,0>F   g6.0<0,1,0>F    g2.0<0,1,0>F { align16 2Q };
mad(8)          g116<1>.xyzF    g9<4,4,1>.xyzzF g6<4,4,1>.xyzzF g30<4,4,1>.xyzzF { align16 NoDDClr 1Q };
mad.le.f0.0(8)  g9<1>F          g3<4,4,1>F      g4.2<0,1,0>F    g15<4,4,1>F { align16 1Q };
mad.le.f0.0(8)  g4<1>F          g2<4,4,1>F      g6.2<0,1,0>F    g20<4,4,1>F { align16 2Q };
mad(8)          g20<1>.xyzF     g8<4,4,1>.xF    g19<4,4,1>.xyzzF -g9<4,4,1>.xF { align16 1Q };
mad(8)          g22<1>.xF       g10<4,4,1>.xF   g21<4,4,1>.xF   (abs)g5.6<0,1,0>F { align16 1Q };
mad.sat(8)      g116<1>.xyzF    g95<4,4,1>.xyzzF g89<4,4,1>.xyzzF g93<4,4,1>.zF { align16 NoDDClr 1Q };
mad(8)          g53<1>F         -g52<4,4,1>F    g21<4,4,1>F     -g21<4,4,1>F { align16 1Q };
mad(8)          g71<1>F         -g8<4,4,1>F     -g2.4<0,1,0>F   -g21<4,4,1>F { align16 1Q };
mad.sat(8)      g40<1>F         g39<4,4,1>F     g37<4,4,1>F     g10<4,4,1>F { align16 1Q };
mad(8)          g67<1>F         g65<4,4,1>F     g6.2<0,1,0>F    -g2.2<0,1,0>F { align16 2Q };
mad(8)          g37<1>F         -g35<4,4,1>F    g87<4,4,1>F     -g87<4,4,1>F { align16 2Q };
mad(8)          g6<1>F          -g93<4,4,1>F    -g2.5<0,1,0>F   -g87<4,4,1>F { align16 2Q };
mad.sat(8)      g9<1>F          g7<4,4,1>F      g118<4,4,1>F    g105<4,4,1>F { align16 2Q };
mad.ge.f0.0(8)  g19<1>.xF       g9<4,4,1>.xF    g18<4,4,1>.xF   -g6.0<0,1,0>F { align16 1Q };
mad(8)          g115<1>.xF      g1<4,4,1>.xF    g9<4,4,1>.xF    g2<4,4,1>.xF { align16 NoDDChk 1Q };
mad.sat(8)      g116<1>.xyzF    -g9<4,4,1>.xyzzF g8<4,4,1>.zxyyF g6<4,4,1>.yzxxF { align16 NoDDClr 1Q };
mad(8)          g125<1>F        g11<4,4,1>F     -g13.0<0,1,0>F  g6<4,4,1>F { align16 1Q };
mad(8)          g123<1>F        g2<4,4,1>F      -g64.0<0,1,0>F  g11<4,4,1>F { align16 2Q };
mad(8)          g2<1>F          -g6<4,4,1>F     (abs)g5<4,4,1>F g14.0<0,1,0>F { align16 1Q };
mad(8)          g2<1>F          -g9<4,4,1>F     (abs)g7<4,4,1>F g64.0<0,1,0>F { align16 2Q };
mad(8)          g11<1>.yF       -g27<4,4,1>.xF  g7.2<0,1,0>F    g6.0<0,1,0>F { align16 NoDDClr,NoDDChk 1Q };
mad(8)          g10<1>.zF       -g30<4,4,1>.xF  g6.6<0,1,0>F    g6.1<0,1,0>F { align16 NoDDChk 1Q };
mad(8)          g5<1>F          -g18.1<0,1,0>F  g7<4,4,1>F      (abs)g2.0<0,1,0>F { align16 1Q };
mad(8)          g5<1>F          -g13.1<0,1,0>F  g12<4,4,1>F     (abs)g2.0<0,1,0>F { align16 2Q };
mad(8)          g6<1>F          g13.0<0,1,0>F   g5<4,4,1>F      (abs)g2.0<0,1,0>F { align16 2Q };
mad.ge.f0.0(8)  g9<1>F          g21.0<0,1,0>F   g7<4,4,1>F      -g2.4<0,1,0>F { align16 2Q };
mad(8)          g13<1>F         g51.2<0,1,0>F   -g51.3<0,1,0>F  (abs)g2.0<0,1,0>F { align16 1Q };
mad(8)          g12<1>F         g31.2<0,1,0>F   -g31.3<0,1,0>F  (abs)g2.0<0,1,0>F { align16 2Q };
mad.l.f0.0(8)   g18<1>F         g4<4,4,1>F      g2.2<0,1,0>F    g2.4<0,1,0>F { align16 1Q };
mad.l.f0.0(8)   g7<1>F          g5<4,4,1>F      g2.2<0,1,0>F    g2.4<0,1,0>F { align16 2Q };
mad(8)          g9<1>.zF        g36<4,4,1>.xF   g27<4,4,1>.xF   g6.7<0,1,0>F { align16 NoDDClr,NoDDChk 1Q };
mad(8)          g5<1>.xF        -g16<4,4,1>.xF  g2.2<0,1,0>F    g1.5<0,1,0>F { align16 NoDDClr 1Q };
mad.nz.f0.0(8)  g10<1>F         -g12.0<0,1,0>F  g7<4,4,1>F      g10<4,4,1>F { align16 1Q };
mad.nz.f0.0(8)  g16<1>F         -g33.0<0,1,0>F  g10<4,4,1>F     g18<4,4,1>F { align16 2Q };