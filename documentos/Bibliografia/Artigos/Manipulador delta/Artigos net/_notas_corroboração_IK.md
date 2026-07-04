# Notas — corroboração das equações de cinemática inversa do manipulador delta

Pesquisa feita para verificar se as equações de IK usadas no firmware (secção "Cinemática inversa do manipulador delta", capítulo 4 do relatório) correspondem a derivações reconhecidas na literatura/comunidade de manipuladores delta, e para tentar localizar a fonte exata de onde partiu o repositório GitHub do isaac879 (indicado como inspiração direta).

## Confirmação direta: isaac879/Delta-Robot

Repositório: https://github.com/isaac879/Delta-Robot
Vídeo: https://www.youtube.com/watch?v=vONuJPu1z3s ("3D Printed Delta Robot (Arduino Controlled)", abril de 2019)

Analisei o ficheiro `delta_robot/deltaRobot.cpp` desse repositório diretamente. O código de IK usado por isaac879 é **estruturalmente idêntico** ao do relatório:

```cpp
float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext)); // lei dos cossenos
float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
float theta = phi + omega;
```

Isto corresponde exatamente a \(\phi_k=\arccos(\ldots)\), \(\omega_k=\operatorname{atan2}(\ldots)\), \(\theta_k=\phi_k+\omega_k\) usado no relatório — mesmos nomes de variável em espírito (`L1`, `ext`≈`d_k`, `l2p`≈`L2`, `SERVO_OFFSET_X`). Os dois ficheiros de imagem do repositório (guardados aqui como `isaac879_diagrama_IK_1_phi_omega_theta.PNG` e `isaac879_diagrama_IK_2_l2pAngle.PNG`) mostram o diagrama CAD anotado com estas mesmas variáveis (phi, omega, theta, L1, l2p, ext, SERVO_OFFSET_X/Z).

**Importante:** procurei extensivamente no repositório (README, comentários no código, descrição do vídeo) e **não encontrei nenhuma citação explícita** de isaac879 a uma fonte externa para esta derivação específica — parece ter sido produzida/adaptada por ele sem referência a terceiros.

## Família 1 — "ângulo de elevação + lei dos cossenos" (mais próxima da do relatório)

### noahctodd — "My delta robot's inverse kinematics" (blog pessoal, 29 jan. 2016)
https://noahctodd.wordpress.com/2016/01/29/my-delta-robots-inverse-kinematics/

Publicado 3 anos antes do vídeo de isaac879. Usa a mesma ideia estrutural — um ângulo de "elevação" somado a um ângulo obtido pela lei dos cossenos:
```
theta = atan( y / z ) + acos( ( pow(h,2) + pow(p1,2) - pow(mp2,2) ) / (2*h*p1) )
```
Nomenclatura diferente (P0–P3, h, mP2) e usa `atan` em vez de `atan2`, mas a mesma "família" de solução (soma de um ângulo de orientação com um ângulo de triângulo via lei dos cossenos) que phi+omega no relatório e no código do isaac879. Não cita fontes.

## Família 2 — "interseção círculo-esfera" (rf, re, 120°) — com origem académica confirmada

### hypertriangle.com/~alex — "Delta robot kinematics" (tutorial)
https://hypertriangle.com/~alex/delta-robot-tutorial/

### Trossen Robotics Community — tutorial de mzavatsky
http://forums.trossenrobotics.com/tutorials/introduction-129/delta-robot-kinematics-3276/
(cópia PDF guardada aqui: `TrossenRobotics_DeltaRobotKinematics_Tutorial.pdf`, via https://github.com/Tomdf/Delta_Robots)

### robottini.altervista.org — "Delta robot with Arduino"
https://robottini.altervista.org/delta-robot-with-arduino
(remete explicitamente para o tutorial da Trossen Robotics acima, em vez de derivar a matemática localmente)

Estes três são, no fundo, a mesma derivação replicada/copiada entre si (mesmas variáveis f, e, rf, re, theta1-3, pontos J/E/F). **Todos remontam à mesma fonte académica original**, citada explicitamente no tutorial de Alex e no de mzavatsky:

### ZsomborMurray2004_DescriptiveGeometricKinematicAnalysis_ClavelDeltaRobot_McGill.pdf
P.J. Zsombor-Murray (McGill University, Centre for Intelligent Machines), *"Descriptive Geometric Kinematic Analysis of Clavel's 'Delta' Robot"*, 1 de abril de 2004.
https://cim.mcgill.ca/~paul/clavdelt.pdf

Este é o artigo académico "raiz" citado por praticamente todos os tutoriais hobbyist de delta robot que circulam online (impressoras 3D delta, Rostock, Kossel, Marlin firmware, etc.). Resolve a IK/FK geometricamente como interseção de reta e esfera.

## Confirmação triangulada da Família 2 (round 2 de pesquisa)

### tinkersprojects_DeltaKinematics_library_source.cpp
William Bailes, biblioteca Arduino *Delta-Kinematics-Library*, tinkersprojects.com.
https://github.com/tinkersprojects/Delta-Kinematics-Library

O código-fonte desta biblioteca **cita explicitamente, no cabeçalho e no README**, exatamente as mesmas quatro fontes já identificadas de forma independente nesta pesquisa:
```
* - http://forums.trossenrobotics.com/tutorials/introduction-129/delta-robot-kinematics-3276/
* - https://www.marginallyclever.com/other/samples/fk-ik-test.html
* - https://github.com/Tomdf/Delta_Robots/blob/master/Diagrams/Delta Robot Kinematics - Trossen Robotics.pdf
* - http://hypertriangle.com/~alex/delta-robot-tutorial/
```
Isto confirma de forma independente (terceira fonte a apontar para o mesmo conjunto) que a "Família 2" é o núcleo mais citado/replicado da comunidade Arduino/impressão 3D para cinemática de delta robot.

### www.marginallyclever.com/other/samples/fk-ik-test.html
Calculadora interativa de FK/IK da Marginally Clever Software, que declara usar "original equations from Trossen Robotics Forums" — mais um nó da mesma árvore (Família 2).

## Outras fontes relacionadas (não delta-específicas, mas do mesmo género de solução)

### hackaday.io — projeto "Deltabot" de deʃhipu (log "Inverse Kinematics")
https://hackaday.io/project/20379-deltabot/log/55077-inverse-kinematics
Resolve por braço com lei dos cossenos, soma de 3 ângulos (sem atan2 — o autor comenta explicitamente que prefere `acos` a `atan2` para este caso). Variante da Família 1, sem citar fontes.

### PMD Corp — white paper corporativo sobre cinemática SCARA/Delta
https://www.pmdcorp.com/resources/type/articles/resources/get/motion-kinematics-article
Artigo técnico da Performance Motion Devices Inc., focado em braços SCARA mas usa exatamente a mesma nomenclatura \(\phi\) (ombro) e \(\theta\) (cotovelo) resolvidos "pela lei dos cossenos e trigonometria básica" — validação da convenção phi/theta como padrão da indústria de controlo de movimento, não apenas do relatório. Sem PDF de download direto disponível.

### YouTube — "Inverse Kinematics: Solving for up-arm and down-arm elbow position" (canal morejpeg)
https://www.youtube.com/watch?v=Cnr4IfHzX2w
Explicador genérico (não específico de delta robot) sobre a ambiguidade de duas soluções (cotovelo para cima/para baixo) ao resolver um ângulo de junta pela lei dos cossenos — o mesmo problema que o relatório resolve ao restringir \(\theta_k\) ao intervalo \([45^\circ,225^\circ]\).

## Nova fonte académica peer-reviewed (2025)

### Hasanlu_Siavashi2025_OptimumKinematicDynamicDeltaRobot_GeneticAlgorithm.pdf
M. Hasanlu (Shanghai Jiao Tong University), M. Siavashi (Babol Noshirvani University of Technology), *"Optimum kinematic–dynamic performance of the reconfigurable delta robot through genetic algorithm optimization"*, Robotic Systems and Applications, vol. 5, n.º 1, 2025.
DOI: https://doi.org/10.21595/rsa.2025.24731

Artigo muito recente (publicado em junho de 2025), confirma que o mesmo problema geométrico — três braços a 120°, comprimentos de braço ativo/passivo, resolução por perna — continua a ser ativamente estudado academicamente. Usa substituição de tangente do meio-ângulo com \(\operatorname{atan2}\) (tal como Williams 2016), aplicada a matrizes de rotação com \(\varphi_i=[0,2\pi/3,4\pi/3]\), o que corresponde diretamente aos ângulos \(\beta_k\in\{0^\circ,120^\circ,240^\circ\}\) do relatório.

## Outras fontes académicas já verificadas anteriormente (mantidas)
- `Williams2016_DeltaParallelRobot_KinematicsSolutions.pdf` — Robert L. Williams II, Ohio University, 2016. Método algébrico (vector loop-closure + tangent half-angle).
- `Hadfield_Wei_Lasenby2020_ForwardInverseKinematicsDeltaRobot_Cambridge.pdf` — Hadfield, Wei, Lasenby (Cambridge), 2020. Método com Álgebra Geométrica Conforme.

## Conclusão

Não existe uma única fonte online que use literalmente a mesma notação exata do relatório (\(\phi_k, \omega_k, \theta_k=\phi_k+\omega_k\), \(L_1, L_2, L_3\)) — o código do isaac879 é o ponto mais próximo, mas ele próprio não cita ninguém. O que a pesquisa confirma é que:

1. A equação usada é uma variante legítima e amplamente usada da família "ângulo de elevação + lei dos cossenos" (Família 1), com pelo menos um precedente publicado 3 anos antes do vídeo de isaac879 (noahctodd, 2016).
2. Esta família é matematicamente equivalente à família "interseção círculo-esfera" (Família 2), que tem uma origem académica clara e citável: Zsombor-Murray (2004, McGill).
3. Para efeitos de comprovação no relatório, a citação academicamente mais defensável a usar é a de **Zsombor-Murray (2004)** como a referência teórica fundadora da cinemática do delta robot por decomposição geométrica, complementada pelas fontes já citadas de Williams (2016) e Hadfield et al. (2020) como validações modernas/alternativas da mesma geometria.
